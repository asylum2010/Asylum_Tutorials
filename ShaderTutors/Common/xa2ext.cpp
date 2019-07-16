
#include "xa2ext.h"

// --- XA2Sound impl ----------------------------------------------------------

XA2Sound::XA2Sound(XA2AudioStreamer* streamer, const std::wstring& file)
{
	creator			= streamer;
	isplaying		= false;
	islooping		= false;
	ismarked		= false;
	freebuffers		= STREAM_BUFFER_COUNT;

	voice			= nullptr;
	reader			= nullptr;
	sample			= nullptr;
	currentbuffer	= 0;
	endofstream		= false;
	readnextsample	= true;

	if (FAILED(MFCreateSourceReaderFromURL(file.c_str(), nullptr, &reader))) {
		std::cout << "XA2Sound::XA2Sound(): Could not create source reader\n";
		throw std::bad_alloc();
	}

	// select first stream
	DWORD stream = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

	reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
	reader->SetStreamSelection(stream, TRUE);

	// check media type
	IMFMediaType* mediatype = nullptr;
	GUID subtype = {};

	reader->GetNativeMediaType(stream, 0, &mediatype);
	mediatype->GetGUID(MF_MT_SUBTYPE, &subtype);

	if (subtype != MFAudioFormat_Float && subtype != MFAudioFormat_PCM) {
		// inform the reader, that we want decoding
		IMFMediaType* decodertype = nullptr;
		MFCreateMediaType(&decodertype);

		decodertype->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		decodertype->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

		reader->SetCurrentMediaType(stream, nullptr, decodertype);
		decodertype->Release();
	}

	mediatype->Release();

	// create wave format
	UINT32 waveformatsize = 0;
	WAVEFORMATEX* waveformat = nullptr;

	reader->GetCurrentMediaType(stream, &mediatype);

	if (FAILED(MFCreateWaveFormatExFromMFMediaType(mediatype, &waveformat, &waveformatsize))) {
		std::cout << "XA2Sound::XA2Sound(): Could not create wave format\n";
		throw std::bad_alloc();
	}

	if (waveformatsize != sizeof(WAVEFORMATEX)) {
		std::cout << "XA2Sound::XA2Sound(): This format is not supported\n";
		throw std::bad_alloc();
	}

	memcpy_s(&format, sizeof(WAVEFORMATEX), waveformat, waveformatsize);

	CoTaskMemFree(waveformat);
	mediatype->Release();

	// create source voice
	if (FAILED(creator->GetInterface()->CreateSourceVoice(&voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this))) {
		std::cout << "XA2Sound::XA2Sound(): Could not create source voice\n";
		throw std::bad_alloc();
	}

	for (int i = 0; i < STREAM_BUFFER_COUNT; ++i) {
		buffers[i] = new BYTE[STREAM_BUFFER_SIZE];
		buffersizes[i] = 0;
	}
}

XA2Sound::~XA2Sound()
{
	islooping = false;
	isplaying = false;
	ismarked = true;

	// wait for the promise to fulfill
	removed.get_future().wait();

	// wait for the queue to empty
	voice->Stop();
	voice->FlushSourceBuffers();

	XAUDIO2_VOICE_STATE state;

	do {
		voice->GetState(&state);
	} while (state.BuffersQueued != 0);

	// now can safely delete members
	if (reader != nullptr) {
		reader->Release();
		reader = nullptr;
	}

	if (voice != nullptr) {
		voice->DestroyVoice();
		voice = nullptr;
	}

	for (int i = 0; i < STREAM_BUFFER_COUNT; ++i) {
		delete[] buffers[i];
	}
}

void XA2Sound::VerifyRemoved(XA2PassKey)
{
	removed.set_value();
}

void XA2Sound::OnBufferEnd(void* context)
{
	UINT32 buffer = *reinterpret_cast<UINT32*>(context);
	delete context;

	if (buffer & XAUDIO2_END_OF_STREAM) {
		if (!islooping) {
			isplaying = false;

			// NOTE: you can't be sure that this method is thread-safe
			voice->Stop();
		}
	}

	buffer &= (~XAUDIO2_END_OF_STREAM);

#if 0 //def _DEBUG
	if (isplaying)
		std::cout << "Buffer " << buffer << " finished\n";
	else
		std::cout << "Buffer " << buffer << " was last\n";
#endif

	buffersizes[buffer] = 0;
	++freebuffers;
}

void XA2Sound::Enqueue()
{
	XAUDIO2_BUFFER xa2buff;
	memset(&xa2buff, 0, sizeof(XAUDIO2_BUFFER));

	if (endofstream)
		xa2buff.Flags = XAUDIO2_END_OF_STREAM;

	xa2buff.pAudioData	= buffers[currentbuffer];
	xa2buff.AudioBytes	= buffersizes[currentbuffer];

	if (xa2buff.AudioBytes > 0) {
		xa2buff.pContext = new UINT32(currentbuffer|xa2buff.Flags);

		if (FAILED(voice->SubmitSourceBuffer(&xa2buff))) {
			std::cout << "XA2Sound::Enqueue(): Could not submit buffer\n";
			delete xa2buff.pContext;
		}
	}
}

void XA2Sound::ResetReader()
{
	PROPVARIANT var = { 0 };
	var.vt = VT_I8;

	endofstream = false;
	reader->SetCurrentPosition(GUID_NULL, var);
}

void XA2Sound::PullData(XA2PassKey)
{
	if (freebuffers == 0)
		return;

	if (endofstream)
		return;

	DWORD stream = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

	// fill current buffer
	do {
		DWORD streamflags = 0;
		DWORD available = STREAM_BUFFER_SIZE - buffersizes[currentbuffer];
		DWORD samplelength = 0;

		if (readnextsample) {
			reader->ReadSample(stream, 0, nullptr, &streamflags, nullptr, &sample);

			if (streamflags & MF_SOURCE_READERF_ENDOFSTREAM) {
				endofstream = true;
				sample = nullptr;
			}
		} else {
			// had a buffer switch last round
			readnextsample = true;
		}

		IMFMediaBuffer* mediabuffer = nullptr;
		BYTE* audiodata = nullptr;

		if (sample != nullptr) {
			sample->ConvertToContiguousBuffer(&mediabuffer);

			mediabuffer->Lock(&audiodata, nullptr, &samplelength);
			{
				if (samplelength <= available) {
					BYTE* dest = buffers[currentbuffer] + buffersizes[currentbuffer];
					memcpy_s(dest, samplelength, audiodata, samplelength);

					buffersizes[currentbuffer] += samplelength;
				} else {
					readnextsample = false;
				}
			}
			mediabuffer->Unlock();
			mediabuffer->Release();
		}
	} while (readnextsample && !endofstream);

	// send to XAudio2
	XAUDIO2_VOICE_STATE state;
	voice->GetState(&state);

	if (state.BuffersQueued < STREAM_BUFFER_COUNT) {
		Enqueue();
	}

	--freebuffers;
	currentbuffer = (currentbuffer + 1) % STREAM_BUFFER_COUNT;

	// reset if looping
	if (endofstream && islooping) {
		ResetReader();
	}
}

void XA2Sound::Play()
{
	ResetReader();

	if (!isplaying) {
		// NOTE: will only play something if you submit data
		voice->Start(0);
	}

	isplaying = true;
	islooping = false;
}

void XA2Sound::Loop()
{
	// NOTE: will only play something if you submit data
	voice->Start(0);

	isplaying = true;
	islooping = true;
}

void XA2Sound::SetVolume(float volume)
{
	voice->SetVolume(volume);
}

// --- XA2AudioStreamer impl --------------------------------------------------

XA2AudioStreamer::XA2AudioStreamer()
{
	xaudio2 = nullptr;
	masteringvoice = nullptr;
	running = true;

	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE))) {
		std::cout << "XA2AudioStreamer::XA2AudioStreamer(): Could not initialize COM library\n";
		throw std::bad_alloc();
	}

	if (FAILED(MFStartup(MF_VERSION))) {
		std::cout << "XA2AudioStreamer::XA2AudioStreamer(): Could not initialize MF library\n";
		throw std::bad_alloc();
	}

	if (FAILED(XAudio2Create(&xaudio2))) {
		std::cout << "XA2AudioStreamer::XA2AudioStreamer(): Could not create XAudio2 object\n";
		throw std::bad_alloc();
	}

	if (FAILED(xaudio2->CreateMasteringVoice(&masteringvoice))) {
		std::cout << "XA2AudioStreamer::XA2AudioStreamer(): Could not create mastering voice\n";
		throw std::bad_alloc();
	}

	worker = std::thread(&XA2AudioStreamer::Run, this);
}

XA2AudioStreamer::~XA2AudioStreamer()
{
	running = false;
	worker.join();

	if (masteringvoice != nullptr)
		masteringvoice->DestroyVoice();

	if (xaudio2 != nullptr)
		xaudio2->Release();

	MFShutdown();
	CoUninitialize();
}

XA2Sound* XA2AudioStreamer::LoadSound(const std::wstring& file)
{
	XA2Sound* sound = new XA2Sound(this, file);
	std::lock_guard<std::mutex> lock(listmutex);

	sounds.push_back(sound);
	return sound;
}

void XA2AudioStreamer::Run()
{
	// NOTE: runs on separate thread
	CoInitializeEx(0, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);

	while (running) {
		bool hasjob = false;

		listmutex.lock();
		{
			auto it = sounds.begin();

			// remove marked sounds
			while (it != sounds.end()) {
				XA2Sound* todel = *it;

				if (todel->IsMarkedForRemove({})) {
					it = sounds.erase(it);
					todel->VerifyRemoved({});
				} else {
					++it;
				}
			}

			// advance others
			for (it = sounds.begin(); it != sounds.end(); ++it) {
				XA2Sound* sound = *it;

				if (sound->IsPlaying()) {
					sound->PullData({});
					hasjob = true;
				}
			}
		}
		listmutex.unlock();

		if (!hasjob)
			Sleep(100);	// 0.1 s
	}

	CoUninitialize();
}
