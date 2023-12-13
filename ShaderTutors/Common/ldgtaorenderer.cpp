
#include "ldgtaorenderer.h"

#define NUM_MIP_LEVELS		5
#define PREFETCH_CACHE_SIZE	8
#define THREADGROUP_SIZE	8

#define HILBERT_LEVEL		6
#define HILBERT_WIDTH		(1 << HILBERT_LEVEL)

static uint16_t HilbertIndex(uint16_t x, uint16_t y)
{
	// https://en.wikipedia.org/wiki/Hilbert_curve

	uint16_t index = 0;

	for (uint16_t level = HILBERT_WIDTH >> 1; level > 0; level >>= 1) {
		uint16_t rx = ((x & level) > 0);
		uint16_t ry = ((y & level) > 0);

		index += level * level * ((3 * rx) ^ ry);

		if (ry == 0) {
			if (rx == 1) {
				x = (HILBERT_WIDTH - 1) - x;
				y = (HILBERT_WIDTH - 1) - y;
			}

			std::swap(x, y);
		}
	}

	return index;
}

LDGTAORenderer::LDGTAORenderer(uint32_t width, uint32_t height)
{
	ldgtaoeffect		= nullptr;
	spatialdenoiser		= nullptr;

	gtaotarget			= 0;
	blurtarget			= 0;
	depthbuffer			= 0;
	ldsLUT				= 0;
	targetWidth			= width;
	targetHeight		= height;
	currsample			= 0;
	enabletemporal		= true;

	// generate LUT
	uint16_t* data = new uint16_t[64 * 64];

	for (uint16_t i = 0; i < 64; ++i) {
		for (uint16_t j = 0; j < 64; ++j) {
			data[i + 64 * j] = HilbertIndex(i, j);
		}
	}

	GL_ASSERT(GLCreateTexture(64, 64, 1, GLFMT_R16_UINT, &ldsLUT, data));
	delete[] data;

	// depth buffers
	float white[] = { 1, 1, 1, 1 };

	GLCreateTexture(width, height, 0, GLFMT_R32F, &depthbuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	// render targets
	GLCreateTexture(width, height, 0, GLFMT_R16F, &gtaotarget);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLCreateTexture(width, height, 0, GLFMT_R8, &blurtarget);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	// load shaders
	char defines[128];
	sprintf_s(defines, "#define NUM_MIP_LEVELS	%d\n#define PREFETCH_CACHE_SIZE	%d\n#define THREADGROUP_SIZE	%d\n\n", NUM_MIP_LEVELS, PREFETCH_CACHE_SIZE, THREADGROUP_SIZE);

	GLCreateComputeProgramFromFile("../../Media/ShadersGL/ldgtao.comp", &ldgtaoeffect, defines);

	ldgtaoeffect->SetInt("gbufferDepth", 0);
	ldgtaoeffect->SetInt("gbufferNormals", 1);
	ldgtaoeffect->SetInt("ldsLUT", 2);
	ldgtaoeffect->SetInt("outputAO", 3);

	GLCreateComputeProgramFromFile("../../Media/ShadersGL/ldgtaodenoise.comp", &spatialdenoiser);

	spatialdenoiser->SetInt("inputAO", 0);
	spatialdenoiser->SetInt("outputAO", 1);
}

LDGTAORenderer::~LDGTAORenderer()
{
	delete ldgtaoeffect;
	delete spatialdenoiser;

	GL_SAFE_DELETE_TEXTURE(ldsLUT);
	GL_SAFE_DELETE_TEXTURE(depthbuffer);
	GL_SAFE_DELETE_TEXTURE(blurtarget);
	GL_SAFE_DELETE_TEXTURE(gtaotarget);
}

void LDGTAORenderer::ClearOnly()
{
	float white[] = { 1, 1, 1, 1 };

	glClearTexImage(gtaotarget, 0, GL_RED, GL_HALF_FLOAT, white);
	glClearTexImage(blurtarget, 0, GL_RED, GL_FLOAT, white);
}

void LDGTAORenderer::Render(GLuint normals, const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector4& clipinfo)
{
	Math::Vector4 projinfo = { 2.0f / (targetWidth * proj._11), 2.0f / (targetHeight * proj._22), -1.0f / proj._11, -1.0f / proj._22 };

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthbuffer);

	// generate depth pyramid
	glGenerateMipmap(GL_TEXTURE_2D);

	// calculate GTAO
	glBindImageTexture(1, normals, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
	glBindImageTexture(2, ldsLUT, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R16UI);
	glBindImageTexture(3, gtaotarget, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R16F);

	ldgtaoeffect->SetVector("projInfo", projinfo);
	ldgtaoeffect->SetVector("clipInfo", clipinfo);

	ldgtaoeffect->Begin();
	{
		glDispatchCompute(targetWidth / THREADGROUP_SIZE, targetHeight / THREADGROUP_SIZE, 1);
	}
	ldgtaoeffect->End();

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// spatial denoiser
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gtaotarget);

	glBindImageTexture(1, blurtarget, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R8UI);

	spatialdenoiser->Begin();
	{
		glDispatchCompute(targetWidth / THREADGROUP_SIZE, targetHeight / THREADGROUP_SIZE, 1);
	}
	spatialdenoiser->End();

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}
