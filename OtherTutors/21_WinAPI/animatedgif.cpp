
#include "animatedgif.h"

// http://www.chimply.com/Generator#classic-spinner

AnimatedGIF::AnimatedGIF(HWND cont)
{
	container		= cont;
	gfx				= NULL;
	stream			= NULL;
	numframes		= 0;
	currentframe	= 0;

	// inherited
	nativeImage		= NULL;
}

AnimatedGIF::~AnimatedGIF()
{
	Dispose();
}

void AnimatedGIF::Dispose()
{
	delete gfx;
	stream->Release();

	if (nativeImage) {
		Gdiplus::DllExports::GdipDisposeImage(nativeImage);
		nativeImage = 0;
	}
}

bool AnimatedGIF::SetResImage(LPCWSTR id)
{
	LPCWSTR pType = L"GIF";
	HMODULE hInst = GetModuleHandle(NULL);
	HRSRC hResource = FindResourceW(hInst, id, pType);

	if (!hResource)
		return false;

	DWORD imageSize = SizeofResource(hInst, hResource);

	if (imageSize == 0)
		return false;

	HGLOBAL hRes = LoadResource(hInst, hResource);
	const void* pResourceData = LockResource(hRes);

	if (!pResourceData)
		return false;

	bool success = SetImage(pResourceData, imageSize);

	UnlockResource(hRes);
	FreeResource(hRes);

	return success;
}

bool AnimatedGIF::SetImage(const void* buff, SIZE_T buffsize)
{
	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, buffsize);
	
	if (hBuffer) {
		void* pBuffer = GlobalLock(hBuffer);

		if (pBuffer)
			CopyMemory(pBuffer, buff, buffsize);

		GlobalUnlock(hBuffer);

		if (stream) {
			stream->Release();
			stream = 0;
		}

		CreateStreamOnHGlobal(hBuffer, FALSE, &stream);
		Gdiplus::DllExports::GdipLoadImageFromStreamICM(stream, &nativeImage);
	}
	
	delete gfx;
	gfx = new Gdiplus::Graphics(container);

	if (!gfx)
		return false;

	// get animation info
	UINT count = 0;
	count = GetFrameDimensionsCount();

	if (count > 0) {
		GUID* dimensions = new GUID[count];
		GetFrameDimensionsList(dimensions, count);

		numframes = GetFrameCount(&dimensions[0]);
		delete[] dimensions;
	}

	InvalidateRect(container, 0, TRUE);
	return true;
}

void AnimatedGIF::Draw()
{
	if (nativeImage) {
		RECT rect;
		GetClientRect(container, &rect);

		LONG width = (rect.right - rect.left);
		LONG height = (rect.bottom - rect.top);

		Gdiplus::RectF srcRect;
		Gdiplus::Unit units;

		GetBounds(&srcRect, &units);

		float wscale = srcRect.Width / (float)width;
		float hscale = srcRect.Height / (float)height;

		if (hscale < wscale) {
			float reducedWidth = hscale * width;

			srcRect.X = (srcRect.Width - reducedWidth) / 2;
			srcRect.Width = reducedWidth;
		} else {
			float reducedHeight = wscale * height;

			srcRect.Y = (srcRect.Height - reducedHeight) / 2;
			srcRect.Height = reducedHeight;
		}

		Gdiplus::Bitmap scaledImage(width, height, PixelFormat32bppRGB);
		Gdiplus::Graphics context(&scaledImage);

		// double buffering
		context.Clear(Gdiplus::Color(240, 240, 240));
		context.DrawImage(this, Gdiplus::RectF((float)rect.left, (float)rect.top, (float)width, (float)height),
			srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, units);

		gfx->DrawImage(
			&scaledImage,
			Gdiplus::RectF((float)rect.left, (float)rect.top, (float)width, (float)height),
			0, 0, (float)width, (float)height, units);
	}
}

void AnimatedGIF::NextFrame()
{
	if (numframes > 0) {
		currentframe = (currentframe + 1) % numframes;

		if (nativeImage)
			SelectActiveFrame(&Gdiplus::FrameDimensionTime, currentframe);
	}
}
