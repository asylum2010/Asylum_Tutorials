
#ifndef _ANIMATEDGIF_H_
#define _ANIMATEDGIF_H_

#include <windows.h>
#include <gdiplus.h>

class AnimatedGIF : public Gdiplus::Image
{
protected:
	Gdiplus::Graphics*	gfx;
	IStream*			stream;
	HWND				container;
	UINT				numframes;
	UINT				currentframe;

public:
	AnimatedGIF(HWND cont);
	~AnimatedGIF();

	bool SetResImage(LPCWSTR id);
	bool SetImage(const void* buff, SIZE_T buffsize);

	void Dispose();
	void Draw();
	void NextFrame();
};

#endif
