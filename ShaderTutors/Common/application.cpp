
#include "application.h"

#ifdef _WIN32
#	include "win32application.h"
#elif defined(__APPLE__)
#	include "macOSapplication.h"
#	include <errno.h>
#	include <stdarg.h>

errno_t fopen_s(FILE** streamptr, const char* filename, const char* mode)
{
	*streamptr = fopen(filename, mode);
	return errno;
}

int fscanf_s(FILE* stream, const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	
	int ret = fscanf(stream, format, argptr);
	va_end(argptr);
	
	return ret;
}
#endif

static void ReadResolutionFile(uint32_t& width, uint32_t& height)
{
	FILE* infile = nullptr;
	
	fopen_s(&infile, "res.conf", "rb");

	if (infile != nullptr) {
		fscanf_s(infile, "%lu %lu\n", &width, &height);
		fclose(infile);

		if (width < 640)
			width = 640;

		if (height < 480)
			height = 480;
	} else {
		fopen_s(&infile, "res.conf", "wb");

		if (infile != nullptr) {
			fprintf(infile, "%ud %ud\n", width, height);
			fclose(infile);
		}
	}
}

Application::~Application()
{
}

Application* Application::Create(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0) {
		// use config file values
		width = 1920;
		height = 1080;

		ReadResolutionFile(width, height);
	}

#ifdef _WIN32
	return new Win32Application(width, height);
#elif defined(__APPLE__)
	return new macOSApplication(width, height);
#else
	return nullptr;
#endif
}
