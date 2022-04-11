
#include "dds.h"
#include "3Dmath.h"

#if defined(VULKAN)
#	include "vk1ext.h"

#	define FORMAT_R8G8B8			VK_FORMAT_B8G8R8_UNORM
#	define FORMAT_B8G8R8			VK_FORMAT_R8G8B8_UNORM
#	define FORMAT_A8R8G8B8			VK_FORMAT_B8G8R8A8_UNORM
#	define FORMAT_DXT1				VK_FORMAT_BC1_RGBA_UNORM_BLOCK
#	define FORMAT_DXT5				VK_FORMAT_BC3_UNORM_BLOCK
#	define FORMAT_G16R16F			VK_FORMAT_R16G16_SFLOAT
#	define FORMAT_A16B16G16R16F		VK_FORMAT_R16G16B16A16_SFLOAT
#	define FORMAT_G32R32F			VK_FORMAT_R32G32_SFLOAT
#elif defined(OPENGL)
#	include "gl4ext.h"

#	define FORMAT_R8G8B8			GLFMT_R8G8B8
#	define FORMAT_B8G8R8			GLFMT_B8G8R8
#	define FORMAT_A8R8G8B8			GLFMT_A8R8G8B8
#	define FORMAT_DXT1				GLFMT_DXT1
#	define FORMAT_DXT5				GLFMT_DXT5
#	define FORMAT_G16R16F			GLFMT_G16R16F
#	define FORMAT_A16B16G16R16F		GLFMT_A16B16G16R16F
#	define FORMAT_G32R32F			GLFMT_G32R32F
#elif defined (METAL)
#import "mtlext.h"

#	define FORMAT_R8G8B8			MTLPixelFormatBGRA8Unorm
#	define FORMAT_B8G8R8			MTLPixelFormatRGBA8Unorm
#	define FORMAT_A8R8G8B8			MTLPixelFormatRGBA8Snorm	// must differentiate somehow from 3-byte formats...
#	define FORMAT_DXT1				MTLPixelFormatBC1_RGBA
#	define FORMAT_DXT5				MTLPixelFormatBC3_RGBA
#	define FORMAT_G16R16F			MTLPixelFormatRG16Float
#	define FORMAT_A16B16G16R16F		MTLPixelFormatRGBA16Float
#	define FORMAT_G32R32F			MTLPixelFormatRG32Float
#endif

#include <cstdio>

#define DWORD							uint32_t
#define WORD							uint16_t
#define BYTE							uint8_t
#define UINT							uint32_t

#define DDS_MAGIC						0x20534444
#define DDPF_FOURCC						0x00000004
#define DDPF_RGB						0x40
#define DDPF_RGBA						0x41

#define DDSD_CAPS						0x01
#define DDSD_HEIGHT						0x02
#define DDSD_WIDTH						0x04
#define DDSD_PITCH						0x08
#define DDSD_PIXELFORMAT				0x1000
#define DDSD_MIPMAPCOUNT				0x20000
#define DDSD_LINEARSIZE					0x80000
#define DDSD_DEPTH						0x800000

#define DDSCAPS_COMPLEX					0x08
#define DDSCAPS_MIPMAP					0x400000
#define DDSCAPS_TEXTURE					0x1000

#define DDSCAPS2_CUBEMAP				0x200
#define DDSCAPS2_CUBEMAP_POSITIVEX		0x400
#define DDSCAPS2_VOLUME					0x200000

#ifndef MAKEFOURCC
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		((DWORD)(BYTE)(ch0)|((DWORD)(BYTE)(ch1) << 8)| \
		((DWORD)(BYTE)(ch2) << 16)|((DWORD)(BYTE)(ch3) << 24))
#endif

struct DDS_PIXELFORMAT
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};

const DDS_PIXELFORMAT DDSPF_DXT1 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','T','1'),
	0, 0, 0, 0, 0
};

/*
const DDS_PIXELFORMAT DDSPF_DXT2 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','T','2'),
	0, 0, 0, 0, 0
};

const DDS_PIXELFORMAT DDSPF_DXT3 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','T','3'),
	0, 0, 0, 0, 0
};

const DDS_PIXELFORMAT DDSPF_DXT4 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','T','4'),
	0, 0, 0, 0, 0
};
*/

const DDS_PIXELFORMAT DDSPF_DXT5 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','T','5'),
	0, 0, 0, 0, 0
};

/*
const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_RGBA, 0, 32,
	0x00ff0000,
	0x0000ff00,
	0x000000ff,
	0xff000000
};

const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_RGBA, 0, 16,
	0x00007c00,
	0x000003e0,
	0x0000001f,
	0x00008000
};

const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_RGBA, 0, 16,
	0x00000f00,
	0x000000f0,
	0x0000000f,
	0x0000f000
};

const DDS_PIXELFORMAT DDSPF_R8G8B8 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_RGB, 0, 24,
	0x00ff0000,
	0x0000ff00,
	0x000000ff,
	0x00000000
};

const DDS_PIXELFORMAT DDSPF_R5G6B5 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_RGB, 0, 16,
	0x0000f800,
	0x000007e0,
	0x0000001f,
	0x00000000
};

const DDS_PIXELFORMAT DDSPF_DX10 =
{
	sizeof(DDS_PIXELFORMAT),
	DDPF_FOURCC,
	MAKEFOURCC('D','X','1','0'),
	0, 0, 0, 0, 0
};
*/

struct DDS_HEADER
{
	DWORD dwSize;
	DWORD dwHeaderFlags;
	DWORD dwHeight;
	DWORD dwWidth;
	DWORD dwPitchOrLinearSize;
	DWORD dwDepth;
	DWORD dwMipMapCount;
	DWORD dwReserved1[11];

	DDS_PIXELFORMAT ddspf;

	DWORD dwCaps;
	DWORD dwCaps2;
	DWORD dwCaps3;
	DWORD dwCaps4;
	DWORD dwReserved2;
};

#ifndef __clang__
static DWORD PackRGBA_DXT1(BYTE r, BYTE g, BYTE b, BYTE a)
{
	return ((a << 24)|(b << 16)|(g << 8)|r);
}

static DWORD PackRGBA_DXT5(BYTE r, BYTE g, BYTE b, BYTE a)
{
	return ((a << 24)|(b << 16)|(g << 8)|r);
}

static void DecompressBlockDXT1(DWORD x, DWORD y, DWORD width, BYTE* blockStorage, DWORD* image)
{
	WORD color0 = *(WORD*)(blockStorage);
	WORD color1 = *(WORD*)(blockStorage + 2);
 	DWORD temp;

	temp = (color0 >> 11) * 255 + 16;
	BYTE r0 = (BYTE)((temp / 32 + temp) / 32);

	temp = ((color0 & 0x07e0) >> 5) * 255 + 32;
	BYTE g0 = (BYTE)((temp / 64 + temp) / 64);

	temp = (color0 & 0x001f) * 255 + 16;
	BYTE b0 = (BYTE)((temp / 32 + temp) / 32);

	temp = (color1 >> 11) * 255 + 16;
	BYTE r1 = (BYTE)((temp / 32 + temp) / 32);

	temp = ((color1 & 0x07e0) >> 5) * 255 + 32;
	BYTE g1 = (BYTE)((temp / 64 + temp) / 64);

	temp = (color1 & 0x001f) * 255 + 16;
	BYTE b1 = (BYTE)((temp / 32 + temp) / 32);

	DWORD code = *(DWORD*)(blockStorage + 4);

	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 4; ++i) {
			DWORD finalColor = 0;
			BYTE positionCode = (code >>  2 * (4 * j + i)) & 0x03;

			if (color0 > color1) {
				switch (positionCode) {
				case 0:
					finalColor = PackRGBA_DXT1(r0, g0, b0, 255);
					break;

				case 1:
					finalColor = PackRGBA_DXT1(r1, g1, b1, 255);
					break;

				case 2:
					finalColor = PackRGBA_DXT1((2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3, 255);
					break;

				case 3:
					finalColor = PackRGBA_DXT1((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3, 255);
					break;
				}
			} else {
				switch (positionCode) {
				case 0:
					finalColor = PackRGBA_DXT1(r0, g0, b0, 255);
					break;

				case 1:
					finalColor = PackRGBA_DXT1(r1, g1, b1, 255);
					break;

				case 2:
					finalColor = PackRGBA_DXT1((r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2, 255);
					break;

				case 3:
					finalColor = PackRGBA_DXT1(0, 0, 0, 255);
					break;
				}
			}

			if ((x + i) < width)
				image[(y + j) * width + (x + i)] = finalColor;
		}
	}
}

static void DecompressBlockDXT5(DWORD x, DWORD y, DWORD width, BYTE* blockStorage, DWORD* image)
{
	BYTE alpha0 = *(BYTE*)(blockStorage);
	BYTE alpha1 = *(BYTE*)(blockStorage + 1);

	BYTE* bits = blockStorage + 2;

	DWORD alphaCode1 = bits[2]|(bits[3] << 8)|(bits[4] << 16)|(bits[5] << 24);
	WORD alphaCode2 = bits[0]|(bits[1] << 8);

	WORD color0 = *(WORD*)(blockStorage + 8);
	WORD color1 = *(WORD*)(blockStorage + 10);

	DWORD temp;

	temp = (color0 >> 11) * 255 + 16;
	BYTE r0 = (BYTE)((temp / 32 + temp) / 32);

	temp = ((color0 & 0x07e0) >> 5) * 255 + 32;
	BYTE g0 = (BYTE)((temp / 64 + temp) / 64);

	temp = (color0 & 0x001f) * 255 + 16;
	BYTE b0 = (BYTE)((temp / 32 + temp) / 32);

	temp = (color1 >> 11) * 255 + 16;
	BYTE r1 = (BYTE)((temp / 32 + temp) / 32);

	temp = ((color1 & 0x07e0) >> 5) * 255 + 32;
	BYTE g1 = (BYTE)((temp / 64 + temp) / 64);

	temp = (color1 & 0x001f) * 255 + 16;
	BYTE b1 = (BYTE)((temp / 32 + temp) / 32);

	DWORD code = *(DWORD*)(blockStorage + 12);

	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 4; ++i) {
			int alphaCodeIndex = 3 * (4 * j + i);
			int alphaCode;

			if (alphaCodeIndex <= 12) {
				alphaCode = (alphaCode2 >> alphaCodeIndex) & 0x07;
			} else if (alphaCodeIndex == 15) {
				alphaCode = (alphaCode2 >> 15) | ((alphaCode1 << 1) & 0x06);
			} else {
				alphaCode = (alphaCode1 >> (alphaCodeIndex - 16)) & 0x07;
			}

			BYTE finalAlpha;

			if (alphaCode == 0) {
				finalAlpha = alpha0;
			} else if (alphaCode == 1) {
				finalAlpha = alpha1;
			} else {
				if (alpha0 > alpha1) {
					finalAlpha = ((8 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 7;
				} else {
					if (alphaCode == 6)
						finalAlpha = 0;
					else if (alphaCode == 7)
						finalAlpha = 255;
					else
						finalAlpha = ((6 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 5;
				}
			}

			BYTE colorCode = (code >> 2 * (4 * j + i)) & 0x03;
			DWORD finalColor = 0;

			switch (colorCode) {
			case 0:
				finalColor = PackRGBA_DXT5(r0, g0, b0, finalAlpha);
				break;

			case 1:
				finalColor = PackRGBA_DXT5(r1, g1, b1, finalAlpha);
				break;

			case 2:
				finalColor = PackRGBA_DXT5((2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3, finalAlpha);
				break;

			case 3:
				finalColor = PackRGBA_DXT5((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3, finalAlpha);
				break;
			}

			if (x + i < width)
				image[(y + j) * width + (x + i)] = finalColor;
		}
	}
}

static void BlockDecompressImageDXT1(DWORD width, DWORD height, BYTE* in, DWORD* out)
{
	DWORD blockCountX = (width + 3) / 4;
	DWORD blockCountY = (height + 3) / 4;

	for (DWORD j = 0; j < blockCountY; ++j) {
		for (DWORD i = 0; i < blockCountX; ++i)
			DecompressBlockDXT1(i * 4, j * 4, width, in + i * 8, out);

		in += blockCountX * 8;
	}
}

static void BlockDecompressImageDXT5(DWORD width, DWORD height, BYTE* in, DWORD* out)
{
	DWORD blockCountX = (width + 3) / 4;
	DWORD blockCountY = (height + 3) / 4;

	for (DWORD j = 0; j < blockCountY; ++j) {
		for (DWORD i = 0; i < blockCountX; ++i)
			DecompressBlockDXT5(i * 4, j * 4, width, in + i * 16, out);

		in += blockCountX * 16;
	}
}
#endif

uint32_t GetImageSize(uint32_t width, uint32_t height, uint32_t bytes, uint32_t miplevels)
{
	uint32_t w = width;
	uint32_t h = height;
	uint32_t bytesize = 0;

	for (uint32_t i = 0; i < miplevels; ++i) {
		bytesize += Math::Max<uint32_t>(1, w) * Math::Max<uint32_t>(1, h) * bytes;

		w = Math::Max<uint32_t>(w / 2, 1);
		h = Math::Max<uint32_t>(h / 2, 1);
	}

	return bytesize;
}

uint32_t GetCompressedImageSize(uint32_t width, uint32_t height, uint32_t miplevels, uint32_t format)
{
	uint32_t w = width;
	uint32_t h = height;
	uint32_t bytesize = 0;

	if (format == FORMAT_DXT1 || format == FORMAT_DXT5) {
		uint32_t mult = ((format == FORMAT_DXT5) ? 16 : 8);

		if (w != h) {
			for (uint32_t i = 0; i < miplevels; ++i) {
				bytesize += Math::Max<uint32_t>(1, w / 4) * Math::Max<uint32_t>(1, h / 4) * mult;

				w = Math::Max<uint32_t>(w / 2, 1);
				h = Math::Max<uint32_t>(h / 2, 1);
			}
		} else {
			bytesize = ((w / 4) * (h / 4) * mult);
			w = bytesize;

			for (uint32_t i = 1; i < miplevels; ++i) {
				w = Math::Max(mult, w / 4);
				bytesize += w;
			}
		}
	}

	return bytesize;
}

uint32_t GetCompressedImageSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t miplevels, uint32_t format)
{
	uint32_t w = width;
	uint32_t h = height;
	uint32_t d = depth;
	uint32_t bytesize = 0;

	if (format == FORMAT_DXT1 || format == FORMAT_DXT5) {
		uint32_t mult = ((format == FORMAT_DXT5) ? 16 : 8);

		for (uint32_t i = 0; i < miplevels; ++i) {
			bytesize += Math::Max<uint32_t>(1, w / 4) * Math::Max<uint32_t>(1, h / 4) * d * mult;

			w = Math::Max<uint32_t>(w / 2, 1);
			h = Math::Max<uint32_t>(h / 2, 1);
		}
	}

	return bytesize;
}

uint32_t GetCompressedLevelSize(uint32_t width, uint32_t height, uint32_t level, uint32_t format)
{
	uint32_t w = width;
	uint32_t h = height;
	uint32_t bytesize = 0;

	if (format == FORMAT_DXT1 || format == FORMAT_DXT5) {
		uint32_t mult = ((format == FORMAT_DXT5) ? 16 : 8);

		if (w != h) {
			w = Math::Max<uint32_t>(w / (1 << level), 1);
			h = Math::Max<uint32_t>(h / (1 << level), 1);

			bytesize = Math::Max<uint32_t>(1, w / 4) * Math::Max<uint32_t>(1, h / 4) * mult;
		} else {
			bytesize = ((w / 4) * (h / 4) * mult);

			if (level > 0) {
				w = bytesize;

				for (uint32_t i = 0; i < level; ++i)
					w = Math::Max<uint32_t>(mult, w / 4);

				bytesize = w;
			}
		}
	}

	return bytesize;
}

uint32_t GetCompressedLevelSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t level, uint32_t format)
{
	uint32_t w = width;
	uint32_t h = height;
	uint32_t bytesize = 0;

	if (format == FORMAT_DXT1 || format == FORMAT_DXT5) {
		uint32_t mult = ((format == FORMAT_DXT5) ? 16 : 8);

		w = Math::Max<uint32_t>(w / (1 << level), 1);
		h = Math::Max<uint32_t>(h / (1 << level), 1);

		bytesize = Math::Max<uint32_t>(1, w / 4) * Math::Max<uint32_t>(1, h / 4) * depth * mult;
	}

	return bytesize;
}

bool LoadFromDDS(const char* file, DDS_Image_Info* outinfo)
{
	DDS_HEADER	header;
	FILE*		infile = 0;
	DWORD		magic;
	UINT		bytesize = 0;

	if (!outinfo)
		return false;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb");
#endif

	if (!infile)
		return false;

	fread(&magic, sizeof(DWORD), 1, infile);

	if (magic != DDS_MAGIC)
		goto _fail;

	fread(&header.dwSize, sizeof(DWORD), 1, infile);
	fread(&header.dwHeaderFlags, sizeof(DWORD), 1, infile);
	fread(&header.dwHeight, sizeof(DWORD), 1, infile);
	fread(&header.dwWidth, sizeof(DWORD), 1, infile);
	fread(&header.dwPitchOrLinearSize, sizeof(DWORD), 1, infile);
	fread(&header.dwDepth, sizeof(DWORD), 1, infile);
	fread(&header.dwMipMapCount, sizeof(DWORD), 1, infile);

	fread(&header.dwReserved1, sizeof(header.dwReserved1), 1, infile);

	fread(&header.ddspf.dwSize, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwFlags, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwFourCC, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwRGBBitCount, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwRBitMask, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwGBitMask, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwBBitMask, sizeof(DWORD), 1, infile);
	fread(&header.ddspf.dwABitMask, sizeof(DWORD), 1, infile);

	fread(&header.dwCaps, sizeof(DWORD), 1, infile);
	fread(&header.dwCaps2, sizeof(DWORD), 1, infile);
	fread(&header.dwCaps3, sizeof(DWORD), 1, infile);
	fread(&header.dwCaps4, sizeof(DWORD), 1, infile);
	fread(&header.dwReserved2, sizeof(DWORD), 1, infile);

	if (header.dwSize != sizeof(DDS_HEADER) || header.ddspf.dwSize != sizeof(DDS_PIXELFORMAT))
		goto _fail;

	outinfo->Width		= header.dwWidth;
	outinfo->Height		= header.dwHeight;
	outinfo->Depth		= header.dwDepth;
	outinfo->Format		= 0;
	outinfo->MipLevels	= (header.dwMipMapCount == 0 ? 1 : header.dwMipMapCount);
	outinfo->Data		= 0;
	outinfo->Type		= DDSImageType2D;

	// TODO: dwRGBBitCount is sometimes zero
	if (header.ddspf.dwFlags & DDPF_FOURCC) {
		if (header.ddspf.dwFourCC == DDSPF_DXT1.dwFourCC) {
			outinfo->Format = FORMAT_DXT1;
		} else if (header.ddspf.dwFourCC == DDSPF_DXT5.dwFourCC) {
			outinfo->Format = FORMAT_DXT5;
		} else if (header.ddspf.dwFourCC == 0x70) {
			outinfo->Format = FORMAT_G16R16F;
			header.ddspf.dwRGBBitCount = 32;
		} else if (header.ddspf.dwFourCC == 0x71) {
			outinfo->Format = FORMAT_A16B16G16R16F;
			header.ddspf.dwRGBBitCount = 64;
		} else if (header.ddspf.dwFourCC == 0x73) {
			outinfo->Format = FORMAT_G32R32F;
			header.ddspf.dwRGBBitCount = 64;
		} else {
			// unsupported
			goto _fail;
		}
	} else if (header.ddspf.dwRGBBitCount == 32) {
		outinfo->Format = FORMAT_A8R8G8B8;
	} else if (header.ddspf.dwRGBBitCount == 24) {
		if (header.ddspf.dwRBitMask & 0x00ff0000) {
			// ARGB (BGRA)
			outinfo->Format = FORMAT_B8G8R8;
		} else {
			// ABGR (RGBA)
			outinfo->Format = FORMAT_R8G8B8;
		}
	} else {
		goto _fail;
	}

	if (header.dwCaps2 & DDSCAPS2_VOLUME) {
		outinfo->Type = DDSImageTypeVolume;

		if (outinfo->Format == FORMAT_DXT1 || outinfo->Format == FORMAT_DXT5) {
			// compressed volume texture
			bytesize = GetCompressedImageSize(outinfo->Width, outinfo->Height, outinfo->Depth, outinfo->MipLevels, outinfo->Format);

			outinfo->Data = malloc(bytesize);
			outinfo->DataSize = bytesize;

			fread(outinfo->Data, 1, bytesize, infile);
		} else {
			// uncompressed volume texture
			bytesize = GetImageSize(outinfo->Width, outinfo->Height, (header.ddspf.dwRGBBitCount / 8), outinfo->MipLevels) * outinfo->Depth;
			outinfo->Data = malloc(bytesize);

			fread((char*)outinfo->Data, 1, bytesize, infile);
			outinfo->DataSize = bytesize;
		}
	} else if (header.dwCaps2 & DDSCAPS2_CUBEMAP) {
		outinfo->Type = DDSImageTypeCube;

		if (outinfo->Format == FORMAT_DXT1 || outinfo->Format == FORMAT_DXT5) {
			// compressed cubemap
			bytesize = GetCompressedImageSize(outinfo->Width, outinfo->Height, outinfo->MipLevels, outinfo->Format) * 6;

			outinfo->Data = malloc(bytesize);
			outinfo->DataSize = bytesize;

			fread(outinfo->Data, 1, bytesize, infile);
		} else {
			// uncompressed cubemap
			bytesize = GetImageSize(outinfo->Width, outinfo->Height, (header.ddspf.dwRGBBitCount / 8), outinfo->MipLevels) * 6;
			outinfo->Data = malloc(bytesize);

			fread((char*)outinfo->Data, 1, bytesize, infile);
			outinfo->DataSize = bytesize;
		}
	} else {
		if (outinfo->Format == FORMAT_DXT1 || outinfo->Format == FORMAT_DXT5) {
			// compressed
			bytesize = GetCompressedImageSize(outinfo->Width, outinfo->Height, outinfo->MipLevels, outinfo->Format);

			outinfo->Data = malloc(bytesize);
			outinfo->DataSize = bytesize;

			fread(outinfo->Data, 1, bytesize, infile);
		} else {
			// uncompressed
			bytesize = GetImageSize(outinfo->Width, outinfo->Height, (header.ddspf.dwRGBBitCount / 8), outinfo->MipLevels);

			outinfo->Data = malloc(bytesize);
			outinfo->DataSize = bytesize;

			fread(outinfo->Data, 1, bytesize, infile);
		}
	}

_fail:
	fclose(infile);

	return (outinfo->Data != 0);
}

bool SaveToDDS(const char* file, const DDS_Image_Info* info)
{
	DDS_HEADER	header;
	FILE*		outfile		= 0;
	DWORD		magic		= DDS_MAGIC;

	if (!info)
		return false;

#ifdef _MSC_VER
	fopen_s(&outfile, file, "wb");
#else
	outfile = fopen(file, "wb");
#endif

	if (!outfile)
		return false;

	memset(&header, 0, sizeof(DDS_HEADER));

	// NOTE: RGBA16F cubemap only
	header.dwSize				= sizeof(DDS_HEADER);
	header.dwHeaderFlags		= 0x2|0x4;
	header.dwHeight				= info->Height;
	header.dwWidth				= info->Width;
	header.dwPitchOrLinearSize	= header.dwWidth * 8;
	header.dwDepth				= 0;
	header.dwMipMapCount		= info->MipLevels;

	header.ddspf.dwSize			= sizeof(DDS_PIXELFORMAT);
	header.ddspf.dwFlags		= 0x4;
	header.ddspf.dwFourCC		= 0x71;
	header.ddspf.dwRGBBitCount	= 64;
	header.ddspf.dwRBitMask		= 0;
	header.ddspf.dwGBitMask		= 0;
	header.ddspf.dwBBitMask		= 0;
	header.ddspf.dwABitMask		= 0;

	header.dwCaps				= 0x00401008;
	header.dwCaps2				= 0xfe00;
	header.dwCaps3				= 0;
	header.dwReserved2			= 0;

	fwrite(&magic, sizeof(DWORD), 1, outfile);
	fwrite(&header, sizeof(DDS_HEADER), 1, outfile);
	fwrite((char*)info->Data, 1, info->DataSize, outfile);

	fclose(outfile);
	return true;
}
