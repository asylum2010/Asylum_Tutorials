
#ifndef _LDGTAORENDERER_H_
#define _LDGTAORENDERER_H_

#include "..\Common\gl4ext.h"

class LDGTAORenderer
{
	// uses low-discrepancy sequence instead of 4x4 texture

private:
	OpenGLEffect*		ldgtaoeffect;
	OpenGLEffect*		spatialdenoiser;

	GLuint				gtaotarget;
	GLuint				blurtarget;
	GLuint				depthbuffer;
	GLuint				ldsLUT;				// lookup texture for low-discrepancy sequence
	uint32_t			targetWidth;
	uint32_t			targetHeight;
	int					currsample;
	bool				enabletemporal;

public:
	LDGTAORenderer(uint32_t width, uint32_t height);
	~LDGTAORenderer();

	void ClearOnly();
	void Render(GLuint normals, const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector4& clipinfo);

	inline void ToggleTemporalDenoiser()			{ enabletemporal = !enabletemporal; }
	inline GLuint GetCurrentDepthBuffer() const		{ return depthbuffer; }
	inline GLuint GetRawGTAO() const				{ return gtaotarget; }
	inline GLuint GetGTAO() const					{ return blurtarget; }
};

#endif
