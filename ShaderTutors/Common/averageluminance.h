
#ifndef _AVERAGELUMINANCE_H_
#define _AVERAGELUMINANCE_H_

#include "gl4ext.h"

class AverageLuminance
{
private:
	OpenGLFramebuffer*	avgluminance;
	OpenGLFramebuffer*	adaptedlumprev;
	OpenGLFramebuffer*	adaptedlumcurr;

	OpenGLScreenQuad*	avgluminitial;
	OpenGLScreenQuad*	avglumiterative;
	OpenGLScreenQuad*	avglumfinal;
	OpenGLScreenQuad*	adaptluminance;

public:
	AverageLuminance();
	~AverageLuminance();

	void Measure(GLuint source, const Math::Vector2& texelsize, float elapsedtime);

	inline GLuint GetAverageLuminance() const	{ return adaptedlumcurr->GetColorAttachment(0); }
};

#endif
