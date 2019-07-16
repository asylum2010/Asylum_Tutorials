
#ifndef _XA2EXT_H_
#define _XA2EXT_H_

// NOTE: https://bell0bytes.eu/streaming-music-with-xaudio2/

#include <iostream>
#include <vector>
#include <thread>
#include <shared_mutex>
#include <future>

// NOTE: don't change the order of includes
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <XAudio2.h>

#define STREAM_BUFFER_SIZE		65536	// 64 KB; shouldn't be larger than XAUDIO2_MAX_BUFFER_BYTES
#define STREAM_BUFFER_COUNT		3

class XA2PassKey
{
	friend class XA2AudioStreamer;
	XA2PassKey() {}
};

class XA2Sound : public IXAudio2VoiceCallback
{
private:
	std::atomic_bool		isplaying;
	std::atomic_bool		islooping;
	std::atomic_bool		ismarked;							// marked for removal
	std::promise<void>		removed;							// promise, that the sound will be removed from XA2AudioStreamer

	std::atomic_int			freebuffers;						// sync between background thread and XAudio2

	WAVEFORMATEX			format;
	IMFSourceReader*		reader;
	IMFSample*				sample;
	IXAudio2SourceVoice*	voice;
	XA2AudioStreamer*		creator;

	BYTE*					buffers[STREAM_BUFFER_COUNT];
	DWORD					buffersizes[STREAM_BUFFER_COUNT];
	int						currentbuffer;						// this changes on every PullData() call
	bool					endofstream;
	bool					readnextsample;						// sometimes a sample doesn't fit into the current buffer

	STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override	{}
	STDMETHOD_(void, OnVoiceProcessingPassEnd)() override			{}
	STDMETHOD_(void, OnStreamEnd)() override						{}
	STDMETHOD_(void, OnBufferStart)(void*) override					{}
	STDMETHOD_(void, OnLoopEnd)(void*) override						{}
	STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override			{}
	STDMETHOD_(void, OnBufferEnd)(void* context) override;

	void Enqueue();
	void ResetReader();

public:
	// protected methods for XA2AudioStreamer
	void PullData(XA2PassKey);
	void VerifyRemoved(XA2PassKey);

	bool IsMarkedForRemove(XA2PassKey) const	{ return ismarked; }

public:
	XA2Sound(XA2AudioStreamer* streamer, const std::wstring& file);
	~XA2Sound();

	void Play();
	void Loop();
	void SetVolume(float volume);

	inline bool IsPlaying() const			{ return isplaying; }
	inline IXAudio2SourceVoice* GetVoice()	{ return voice; }
};

class XA2AudioStreamer
{
	typedef std::vector<XA2Sound*> SoundList;

private:
	std::thread				worker;
	std::mutex				listmutex;	// to guard the sound list

	SoundList				sounds;
	IXAudio2*				xaudio2;
	IXAudio2MasteringVoice*	masteringvoice;
	std::atomic_bool		running;

	void Run();

public:
	XA2AudioStreamer();
	~XA2AudioStreamer();

	XA2Sound* LoadSound(const std::wstring& file);

	inline IXAudio2* GetInterface()	{ return xaudio2; }
};

#endif
