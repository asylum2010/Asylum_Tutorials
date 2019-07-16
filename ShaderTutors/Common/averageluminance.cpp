
#include "averageluminance.h"

AverageLuminance::AverageLuminance()
{
	avgluminance = new OpenGLFramebuffer(64, 64);
	avgluminance->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R16F, GL_NEAREST);

	glGenerateMipmap(GL_TEXTURE_2D);

	avgluminance->Validate();

	adaptedlumprev = new OpenGLFramebuffer(1, 1);
	adaptedlumcurr = new OpenGLFramebuffer(1, 1);

	adaptedlumprev->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R16F, GL_NEAREST);
	adaptedlumcurr->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R16F, GL_NEAREST);

	adaptedlumprev->Validate();
	adaptedlumcurr->Validate();

	adaptedlumcurr->Set();
	{
		float expected[] = { 0.167f, 0.167f, 0.167f, 1 };
		glClearBufferfv(GL_COLOR, 0, expected);
	}
	adaptedlumcurr->Unset();

	// load shaders
	Math::Matrix identity(1, 1, 1, 1);

	avgluminitial = new OpenGLScreenQuad("../../Media/ShadersGL/avglum.frag", "#define SAMPLE_MODE 0\n");
	avglumiterative = new OpenGLScreenQuad("../../Media/ShadersGL/avglum.frag", "#define SAMPLE_MODE 1\n");
	avglumfinal = new OpenGLScreenQuad("../../Media/ShadersGL/avglum.frag", "#define SAMPLE_MODE 2\n");
	adaptluminance = new OpenGLScreenQuad("../../Media/ShadersGL/adaptlum.frag");

	avgluminitial->SetTextureMatrix(identity);
	avglumiterative->SetTextureMatrix(identity);
	avglumfinal->SetTextureMatrix(identity);
	adaptluminance->SetTextureMatrix(identity);

	adaptluminance->GetEffect()->SetInt("sampler1", 1);
}

AverageLuminance::~AverageLuminance()
{
	delete avgluminance;
	delete adaptedlumprev;
	delete adaptedlumcurr;

	delete avgluminitial;
	delete avglumiterative;
	delete avglumfinal;
	delete adaptluminance;
}

void AverageLuminance::Measure(GLuint source, const Math::Vector2& texelsize, float elapsedtime)
{
	avgluminance->Set();
	{
		// 64x64
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, source);

		avgluminitial->GetEffect()->SetVector("texelSize", texelsize);
		avgluminitial->Draw();

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, avgluminance->GetColorAttachment(0));

		// 16x16
		avgluminance->Attach(GL_COLOR_ATTACHMENT0, avgluminance->GetColorAttachment(0), 2);
		glViewport(0, 0, 16, 16);

		avglumiterative->GetEffect()->SetInt("prevLevel", 0);
		avglumiterative->Draw();

		// 4x4
		avgluminance->Attach(GL_COLOR_ATTACHMENT0, avgluminance->GetColorAttachment(0), 4);
		glViewport(0, 0, 4, 4);

		avglumiterative->GetEffect()->SetInt("prevLevel", 2);
		avglumiterative->Draw();

		// 1x1
		avgluminance->Attach(GL_COLOR_ATTACHMENT0, avgluminance->GetColorAttachment(0), 6);
		glViewport(0, 0, 1, 1);

		avglumfinal->GetEffect()->SetInt("prevLevel", 4);
		avglumfinal->Draw();

		avgluminance->Attach(GL_COLOR_ATTACHMENT0, avgluminance->GetColorAttachment(0), 0);
	}
	avgluminance->Unset();

	// adapt
	std::swap(adaptedlumcurr, adaptedlumprev);

	adaptedlumcurr->Set();
	{
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, adaptedlumprev->GetColorAttachment(0));
		GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, avgluminance->GetColorAttachment(0));

		adaptluminance->GetEffect()->SetFloat("elapsedTime", elapsedtime);
		adaptluminance->Draw();

#if 0
		Math::Float16 bits;
		glReadPixels(0, 0, 1, 1, GL_RED, GL_HALF_FLOAT, &bits);

		float avglum = (float)bits;
		float exposure = 1.0f / (9.6f * avglum);

		printf("avg lum: %.3f, exposure: %.3f\n", avglum, exposure);
#endif
	}
	adaptedlumcurr->Unset();
}
