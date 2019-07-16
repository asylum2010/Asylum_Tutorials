
#ifndef _RENDERINGCORE_H_
#define _RENDERINGCORE_H_

#include <thread>
#include <atomic>

#include "../Common/blockingqueue.hpp"
#include "../Common/gl4ext.h"

#define SAFE_DELETE(x)	{ if ((x)) { delete (x); (x) = 0; } }

class IRenderingContext;

/**
 * \brief Abstract task class
 */
class IRenderingTask
{
private:
	std::atomic_bool disposing;

protected:
	int	 universeid;

public:
	IRenderingTask(int universe);
	virtual ~IRenderingTask();

	virtual void Execute(IRenderingContext* context) = 0;
	virtual void Dispose() = 0;

	inline void MarkForDispose()			{ disposing = true; }
	inline bool IsMarkedForDispose () const	{ return disposing; }
	inline int GetUniverseID() const		{ return universeid; }
};

/**
 * \brief This is what is visible to the public
 *
 * Can be used from the renderer thread only (a.k.a. tasks).
 */
class IRenderingContext
{
public:
	virtual ~IRenderingContext();

	// factory methods
	virtual OpenGLFramebuffer*	CreateFramebuffer(GLuint width, GLuint height) = 0;
	virtual OpenGLScreenQuad*	CreateScreenQuad() = 0;
	virtual OpenGLEffect*		CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile) = 0;
	virtual OpenGLMesh*			CreateMesh(const char* file) = 0;
	virtual OpenGLMesh*			CreateMesh(GLuint numvertices, GLuint numindices, GLuint flags, OpenGLVertexElement* decl) = 0;

	// renderstate methods
	virtual void SetBlendMode(GLenum src, GLenum dst) = 0;
	virtual void SetCullMode(GLenum mode) = 0;
	virtual void SetDepthTest(GLboolean enable) = 0;
	virtual void SetDepthFunc(GLenum func) = 0;

	// rendering methods
	virtual void Blit(OpenGLFramebuffer* from, OpenGLFramebuffer* to, GLbitfield flags) = 0;
	virtual void Clear(GLbitfield target, const Math::Color& color, float depth = 1.0f) = 0;
	virtual void Present(int id) = 0;

	// other
	virtual void CheckError() = 0;
};

/**
 * \brief Singleton thread for all OpenGL calls
 */
class RenderingCore
{
	friend RenderingCore* GetRenderingCore();

public:
	class PrivateInterface;

	void AddTask(IRenderingTask* task);
	void Shutdown();

	// these methods always block
	int CreateUniverse(HDC hdc);
	void DeleteUniverse(int id);
	void Barrier();

private:
	static RenderingCore* _inst;
	static std::mutex singletonguard;

	std::thread						thread;
	BlockingQueue<IRenderingTask*>	tasks;
	PrivateInterface*				privinterf;

	RenderingCore();
	~RenderingCore();

	bool InitializeCoreProfile();
	void THREAD_Run();
};

RenderingCore* GetRenderingCore();

#endif
