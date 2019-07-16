
#include "gtaorenderer.h"

#define NUM_MIP_LEVELS		5
#define PREFETCH_CACHE_SIZE	8

GTAORenderer::GTAORenderer(uint32_t width, uint32_t height)
{
	gtaotarget			= nullptr;
	blurtarget			= nullptr;
	accumtargets[0]		= nullptr;
	accumtargets[1]		= nullptr;

	depthpyramid		= nullptr;
	gtaoeffect			= nullptr;
	spatialdenoiser		= nullptr;
	temporaldenoiser	= nullptr;

	screenquad			= new OpenGLScreenQuad(nullptr);

	depthbuffers[0]		= 0;
	depthbuffers[1]		= 0;
	noisetex			= 0;
	currsample			= 0;

	// generate noise texture
	uint8_t* data = new uint8_t[16 * 2];

	for (uint8_t i = 0; i < 4; ++i) {
		for (uint8_t j = 0; j < 4; ++j) {
			float dirnoise = 0.0625f * ((((i + j) & 0x3) << 2) + (i & 0x3));
			float offnoise = 0.25f * ((j - i) & 0x3);

			data[(i * 4 + j) * 2 + 0] = (uint8_t)(dirnoise * 255.0f);
			data[(i * 4 + j) * 2 + 1] = (uint8_t)(offnoise * 255.0f);
		}
	}

	GL_ASSERT(GLCreateTexture(4, 4, 1, GLFMT_R8G8, &noisetex, data));
	delete[] data;

	// depth buffers
	float white[] = { 1, 1, 1, 1 };

	GLCreateTexture(width, height, 0, GLFMT_R32F, &depthbuffers[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	GLCreateTexture(width, height, 0, GLFMT_R32F, &depthbuffers[1]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	// render targets
	GLint value = 0;

	pyramidtarget = new OpenGLFramebuffer(width / 2, height / 2);

	gtaotarget = new OpenGLFramebuffer(width, height);
	gtaotarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R16F, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_ASSERT(gtaotarget->Validate());

	glBindFramebuffer(GL_FRAMEBUFFER, gtaotarget->GetFramebuffer());
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &value);
		GL_ASSERT(value == GL_LINEAR);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	blurtarget = new OpenGLFramebuffer(width, height);
	blurtarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R8G8, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	GL_ASSERT(blurtarget->Validate());

	glBindFramebuffer(GL_FRAMEBUFFER, blurtarget->GetFramebuffer());
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &value);
		GL_ASSERT(value == GL_LINEAR);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// accumulation targets
	accumtargets[0] = new OpenGLFramebuffer(width, height);
	accumtargets[0]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R8G8, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	GL_ASSERT(accumtargets[0]->Validate());

	accumtargets[1] = new OpenGLFramebuffer(width, height);
	accumtargets[1]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R8G8, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	GL_ASSERT(accumtargets[1]->Validate());

	// load shaders
	char defines[128];
	sprintf_s(defines, "#define NUM_MIP_LEVELS %d\n#define PREFETCH_CACHE_SIZE %d\n", NUM_MIP_LEVELS, PREFETCH_CACHE_SIZE);

	GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/depthpyramid.frag", &depthpyramid);
	GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtao.frag", &gtaoeffect, defines);
	GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtaospatialdenoiser.frag", &spatialdenoiser);
	GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtaotemporaldenoiser.frag", &temporaldenoiser);

	Math::Matrix identity;
	Math::MatrixIdentity(identity);

	depthpyramid->SetMatrix("matTexture", identity);
	gtaoeffect->SetMatrix("matTexture", identity);
	spatialdenoiser->SetMatrix("matTexture", identity);
	temporaldenoiser->SetMatrix("matTexture", identity);

	gtaoeffect->SetInt("gbufferDepth", 0);
	gtaoeffect->SetInt("gbufferNormals", 1);
	gtaoeffect->SetInt("noise", 2);

	spatialdenoiser->SetInt("gtao", 0);
	spatialdenoiser->SetInt("depth", 1);
	
	temporaldenoiser->SetInt("historyBuffer", 0);
	temporaldenoiser->SetInt("currIteration", 1);
	temporaldenoiser->SetInt("prevDepthBuffer", 2);
	temporaldenoiser->SetInt("currDepthBuffer", 3);

	Math::MatrixIdentity(prevview);
	Math::MatrixIdentity(currview);
}

GTAORenderer::~GTAORenderer()
{
	delete depthpyramid;
	delete gtaoeffect;
	delete spatialdenoiser;
	delete temporaldenoiser;

	delete pyramidtarget;
	delete gtaotarget;
	delete blurtarget;
	delete accumtargets[0];
	delete accumtargets[1];

	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(depthbuffers[0]);
	GL_SAFE_DELETE_TEXTURE(depthbuffers[1]);
	GL_SAFE_DELETE_TEXTURE(noisetex);
}

void GTAORenderer::Render(GLuint normals, const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector3& eye, const Math::Vector4& clipinfo)
{
	const float rotations[6] = { 60.0f, 300.0f, 180.0f, 240.0f, 120.0f, 0.0f };
	const float offsets[4] = { 0.0f, 0.5f, 0.25f, 0.75f };

	uint32_t width			= gtaotarget->GetWidth();
	uint32_t height			= gtaotarget->GetHeight();

	Math::Vector4 projinfo	= { 2.0f / (width * proj._11), 2.0f / (height * proj._22), -1.0f / proj._11, -1.0f / proj._22 };
	Math::Vector4 params	= { 0, 0, 0, 0 };

	OpenGLFramebuffer* srctarget = accumtargets[currsample % 2];
	OpenGLFramebuffer* dsttarget = accumtargets[1 - currsample % 2];

	currview = view;

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	GLuint depthtex = depthbuffers[currsample % 2];

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthtex);

	// generate depth pyramid
	for (int i = 1; i < NUM_MIP_LEVELS; ++i) {
		pyramidtarget->Attach(GL_COLOR_ATTACHMENT0, depthtex, i);
		pyramidtarget->Set();
		{
			depthpyramid->SetInt("prevMipLevel", i - 1);
			depthpyramid->Begin();
			{
				screenquad->Draw(false);	
			}
			depthpyramid->End();
		}
		pyramidtarget->Unset();
		pyramidtarget->Detach(GL_COLOR_ATTACHMENT0);
	}

	// calculate GTAO
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normals);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noisetex);

	params.x = rotations[currsample % 6] / 360.0f;
	params.y = offsets[(currsample / 6) % 4];

	gtaoeffect->SetVector("eyePos", eye);
	gtaoeffect->SetVector("projInfo", projinfo);
	gtaoeffect->SetVector("clipInfo", clipinfo);
	gtaoeffect->SetVector("params", params);

	gtaotarget->Set();
	{
		gtaoeffect->Begin();
		{
			screenquad->Draw(false);
		}
		gtaoeffect->End();
	}
	gtaotarget->Unset();

	// spatial denoiser
	Math::Matrix invprevview;
	Math::Matrix invcurrview;
	float backprojinfo[] = { 1.0f / proj._11, 1.0f / proj._22, 0, 0 };

	Math::MatrixInverse(invprevview, prevview);
	Math::MatrixInverse(invcurrview, currview);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gtaotarget->GetColorAttachment(0));

	blurtarget->Set();
	{
		spatialdenoiser->Begin();
		{
			screenquad->Draw(false);
		}
		spatialdenoiser->End();
	}
	blurtarget->Unset();

	// temporal denoiser
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srctarget->GetColorAttachment(0));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurtarget->GetColorAttachment(0));

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, depthbuffers[1 - currsample % 2]);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthbuffers[currsample % 2]);

	dsttarget->Set();
	{
		temporaldenoiser->SetMatrix("matPrevViewInv", invprevview);
		temporaldenoiser->SetMatrix("matCurrViewInv", invcurrview);
		temporaldenoiser->SetMatrix("matPrevView", prevview);
		temporaldenoiser->SetMatrix("matProj", proj);

		temporaldenoiser->SetVector("projInfo", backprojinfo);
		temporaldenoiser->SetVector("clipPlanes", clipinfo);

		temporaldenoiser->Begin();
		{
			screenquad->Draw(false);
		}
		temporaldenoiser->End();
	}
	dsttarget->Unset();

	// prepare for next round
	prevview = currview;
	currsample = (currsample + 1) % 6;
}
