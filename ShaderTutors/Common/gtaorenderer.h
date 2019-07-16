
#ifndef _GTAORENDERER_H_
#define _GTAORENDERER_H_

#include "..\Common\gl4ext.h"

class GTAORenderer
{
private:
	Math::Matrix		prevview;			// for temporal reprojection
	Math::Matrix		currview;			// for temporal reprojection

	OpenGLFramebuffer*	pyramidtarget;
	OpenGLFramebuffer*	gtaotarget;
	OpenGLFramebuffer*	blurtarget;
	OpenGLFramebuffer*	accumtargets[2];

	OpenGLEffect*		depthpyramid;		// for optimization
	OpenGLEffect*		gtaoeffect;
	OpenGLEffect*		spatialdenoiser;
	OpenGLEffect*		temporaldenoiser;

	OpenGLScreenQuad*	screenquad;

	GLuint				depthbuffers[2];	// for temporal denoiser
	GLuint				noisetex;
	int					currsample;

public:
	GTAORenderer(uint32_t width, uint32_t height);
	~GTAORenderer();

	void Render(GLuint normals, const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector3& eye, const Math::Vector4& clipinfo);

	inline GLuint GetCurrentDepthBuffer() const		{ return depthbuffers[currsample % 2]; }
	inline GLuint GetGTAO() const					{ return accumtargets[currsample % 2]->GetColorAttachment(0); }
	inline GLuint GetRawGTAO() const				{ return gtaotarget->GetColorAttachment(0); }
};

#endif
