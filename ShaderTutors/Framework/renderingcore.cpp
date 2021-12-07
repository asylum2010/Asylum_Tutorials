
#include <iostream>
#include <future>

#include "renderingcore.h"

#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

RenderingCore*	RenderingCore::_inst = nullptr;
std::mutex		RenderingCore::singletonguard;

RenderingCore* GetRenderingCore()
{
	RenderingCore::singletonguard.lock();
	{
		if (RenderingCore::_inst == nullptr)
			RenderingCore::_inst = new RenderingCore();
	}
	RenderingCore::singletonguard.unlock();

	return RenderingCore::_inst;
}

IRenderingContext::~IRenderingContext()
{
}

static void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
	if (type >= GL_DEBUG_TYPE_ERROR && type <= GL_DEBUG_TYPE_PERFORMANCE) {
		if (source == GL_DEBUG_SOURCE_API)
			std::cout << "GL(" << severity << "): ";
		else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER)
			std::cout << "GLSL(" << severity << "): ";
		else
			std::cout << "OTHER(" << severity << "): ";

		std::cout << id << ": " << message << "\n";
	}

	if (type == GL_DEBUG_TYPE_ERROR)
		__debugbreak();
}

// --- PrivateInterface definition --------------------------------------------

class RenderingCore::PrivateInterface : public IRenderingContext
{
	friend class UniverseCreatorTask;
	friend class RenderingCore;

	struct OpenGLContext
	{
		HDC hdc;
		HGLRC hrc;

		OpenGLContext() {
			hdc = nullptr;
			hrc = nullptr;
		}
	};

	typedef std::vector<OpenGLContext> ContextArray;

private:
	ContextArray	contexts;
	OpenGLContext	activecontext;

	// internal methods
	int CreateContext(HDC hdc);
	void DeleteContext(int id);
	bool ActivateContext(int id);
	HDC GetDC(int id) const;

public:
	// factory methods
	OpenGLFramebuffer*	CreateFramebuffer(GLuint width, GLuint height) override;
	OpenGLScreenQuad*	CreateScreenQuad() override;
	OpenGLEffect*		CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile) override;
	OpenGLMesh*			CreateMesh(const char* file) override;
	OpenGLMesh*			CreateMesh(GLuint numvertices, GLuint numindices, GLuint flags, OpenGLVertexElement* decl) override;

	// renderstate methods
	void SetBlendMode(GLenum src, GLenum dst) override;
	void SetCullMode(GLenum mode) override;
	void SetDepthTest(GLboolean enable) override;
	void SetDepthFunc(GLenum func) override;

	// rendering methods
	void Blit(OpenGLFramebuffer* from, OpenGLFramebuffer* to, GLbitfield flags) override;
	void Clear(GLbitfield target, const Math::Color& color, float depth) override;
	void Present(int id) override;

	// other
	void CheckError();
};

// --- PrivateInterface impl --------------------------------------------------

int RenderingCore::PrivateInterface::CreateContext(HDC hdc)
{
	OpenGLContext	context;
	int				pixelformat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24, // color
		0, 0, 0, 0, 0, 0,
		0, // alpha
		0, 0, 0, 0, 0, 0,
		24, 8, 0, // depth, stencil, aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	auto Cleanup = [&]() {
		if (context.hrc != nullptr) {
			wglMakeCurrent(hdc, NULL);
			wglDeleteContext(context.hrc);
		}

		context.hdc = nullptr;
	};

	context.hrc = nullptr;
	context.hdc = nullptr;

	if (GLExtensions::GLVersion < GLExtensions::GL_3_3) {
		MessageBox(NULL, "Core profile not supported on this device", "Fatal error", MB_OK);
		Cleanup();

		return -1;
	}

	if (!GLExtensions::WGL_ARB_pixel_format || !GLExtensions::WGL_ARB_create_context || !GLExtensions::WGL_ARB_create_context_profile) {
		MessageBox(NULL, "WGL_ARB_pixel_format not found", "Fatal error", MB_OK);
		Cleanup();

		return -1;
	}

	// create core profile context
	int attrib[32];
	int i = 0;
	UINT numformats = 0;

	memset(attrib, 0, sizeof(attrib));

	attrib[i++] = 0x2001;		// WGL_DRAW_TO_WINDOW_ARB
	attrib[i++] = TRUE;
	attrib[i++] = 0x2003;		// WGL_ACCELERATION_ARB
	attrib[i++] = 0x2027;		// WGL_FULL_ACCELERATION_ARB
	attrib[i++] = 0x2010;		// WGL_SUPPORT_OPENGL_ARB
	attrib[i++] = TRUE;
	attrib[i++] = 0x2011;		// WGL_DOUBLE_BUFFER_ARB
	attrib[i++] = TRUE;
	attrib[i++] = 0x2013;		// WGL_PIXEL_TYPE_ARB
	attrib[i++] = 0x202B;		// WGL_TYPE_RGBA_ARB
	attrib[i++] = 0x2014;		// WGL_COLOR_BITS_ARB
	attrib[i++] = pfd.cColorBits = 32;
	attrib[i++] = 0x201B;		// WGL_ALPHA_BITS_ARB
	attrib[i++] = pfd.cAlphaBits = 0;
	attrib[i++] = 0x2022;		// WGL_DEPTH_BITS_ARB
	attrib[i++] = pfd.cDepthBits = 24;
	attrib[i++] = 0x2023;		// WGL_STENCIL_BITS_ARB
	attrib[i++] = pfd.cStencilBits = 8;
	attrib[i++] = 0;

	if (attrib[1])
		pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

	if (attrib[5])
		pfd.dwFlags |= PFD_SUPPORT_OPENGL;

	if (attrib[7])
		pfd.dwFlags |= PFD_DOUBLEBUFFER;

	if (attrib[9] == 0x2029)	// WGL_SWAP_COPY_ARB
		pfd.dwFlags |= PFD_SWAP_COPY;

	wglChoosePixelFormat(hdc, attrib, 0, 1, &pixelformat, &numformats);

	if (0 == numformats) {
		MessageBox(NULL, "wglChoosePixelFormat failed", "Fatal error", MB_OK);
		Cleanup();

		return -1;
	}

	std::cout << "Selected pixel format: " << pixelformat <<"\n";

	int success = SetPixelFormat(hdc, pixelformat, &pfd);

	if (0 == success) {
		MessageBox(NULL, "Could not set pixel format", "Fatal error", MB_OK);
		Cleanup();

		return -1;
	}

	int contextattribs[] = {
		0x2091,			// WGL_CONTEXT_MAJOR_VERSION_ARB
		GLExtensions::GetMajorVersion(),
		0x2092,			// WGL_CONTEXT_MINOR_VERSION_ARB
		GLExtensions::GetMinorVersion(),
		0x2094,			// WGL_CONTEXT_FLAGS_ARB
#ifdef _DEBUG
		0x0001,			// WGL_CONTEXT_DEBUG_BIT
#else
		0,
#endif
		0x9126,			// WGL_CONTEXT_PROFILE_MASK_ARB
		0x00000001,		// WGL_CONTEXT_CORE_PROFILE_BIT_ARB
		0
	};

	if (contexts.size() > 0)
		context.hrc = wglCreateContextAttribs(hdc, contexts[0].hrc, contextattribs);
	else
		context.hrc = wglCreateContextAttribs(hdc, NULL, contextattribs);

	if (context.hrc == nullptr) {
		MessageBox(NULL, "Could not create OpenGL core profile context", "Fatal error", MB_OK);
		Cleanup();

		return -1;
	}

	context.hdc = hdc;

	std::cout << "Created core profile context (" << contexts.size() << ")\n";
	wglMakeCurrent(hdc, context.hrc);

	if (GLExtensions::WGL_EXT_swap_control)
		wglSwapInterval(1);

	if (GLExtensions::GLVersion >= GLExtensions::GL_4_3) {
		glEnable(GL_DEBUG_OUTPUT);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}

	contexts.push_back(context);
	activecontext = context;

	return (int)contexts.size() - 1;
}

void RenderingCore::PrivateInterface::DeleteContext(int id)
{
	OpenGLContext& context = contexts[id];

	if (context.hrc != nullptr) {
		bool isthisactive = (context.hrc == activecontext.hrc);

		if (isthisactive) {
			if (!wglMakeCurrent(context.hdc, NULL))
				MYERROR("Could not release context");
		}

		if (!wglDeleteContext(context.hrc))
			MYERROR("Could not delete context");

		std::cout << "Deleted context (" << id << ")\n";

		context.hdc = nullptr;
		context.hrc = nullptr;

		if (isthisactive)
			activecontext = context;
	}
}

bool RenderingCore::PrivateInterface::ActivateContext(int id)
{
	if (id < 0 || id >= (int)contexts.size())
		return false;

	OpenGLContext& context = contexts[id];

	if (activecontext.hrc != context.hrc) {
		if (context.hrc != nullptr) {
			if (!wglMakeCurrent(context.hdc, context.hrc)) {
				MYERROR("Could not activate context");
				return false;
			}

			activecontext = context;
		}
	}

	return true;
}

HDC RenderingCore::PrivateInterface::GetDC(int id) const
{
	if (id < 0 || id >= (int)contexts.size())
		return 0;

	const OpenGLContext& context = contexts[id];
	return context.hdc;
}

void RenderingCore::PrivateInterface::SetBlendMode(GLenum src, GLenum dst)
{
	if (src == GL_ZERO && dst == GL_ZERO) {
		glDisable(GL_BLEND);
	} else {
		glEnable(GL_BLEND);
		glBlendFunc(src, dst);
	}
}

void RenderingCore::PrivateInterface::SetCullMode(GLenum mode)
{
	if (mode == GL_NONE) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(mode);
	}
}

void RenderingCore::PrivateInterface::SetDepthTest(GLboolean enable)
{
	if (enable == GL_TRUE)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void RenderingCore::PrivateInterface::SetDepthFunc(GLenum func)
{
	glDepthFunc(func);
}

void RenderingCore::PrivateInterface::Blit(OpenGLFramebuffer* from, OpenGLFramebuffer* to, GLbitfield flags)
{
	if (!from || !to || from == to || flags == 0)
		return;

	GLenum filter = GL_LINEAR;

	if (flags & GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT)
		filter = GL_NEAREST;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, from->GetFramebuffer());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->GetFramebuffer());

	glBlitFramebuffer(
		0, 0, from->GetWidth(), from->GetHeight(),
		0, 0, to->GetWidth(), to->GetHeight(),
		flags, filter);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingCore::PrivateInterface::Clear(GLbitfield target, const Math::Color& color, float depth)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClearDepth(depth);
	glClear(target);
}

void RenderingCore::PrivateInterface::Present(int id)
{
	if (id < 0 || id >= (int)contexts.size())
		return;

	const OpenGLContext& context = contexts[id];

	if (context.hdc != nullptr)
		SwapBuffers(context.hdc);
}

void RenderingCore::PrivateInterface::CheckError()
{
	GLenum err = glGetError();

	switch (err) {
	case GL_NO_ERROR:
		break;

	case GL_INVALID_OPERATION:
		std::cout << "RenderingCore::PrivateInterface::CheckError(): GL_INVALID_OPERATION\n";
		break;

	case GL_INVALID_ENUM:
		std::cout << "RenderingCore::PrivateInterface::CheckError(): GL_INVALID_ENUM\n";
		break;

	case GL_INVALID_VALUE:
		std::cout << "RenderingCore::PrivateInterface::CheckError(): GL_INVALID_VALUE\n";
		break;

	case GL_OUT_OF_MEMORY:
		std::cout << "RenderingCore::PrivateInterface::CheckError(): GL_OUT_OF_MEMORY\n";
		break;

	default:
		std::cout << "RenderingCore::PrivateInterface::CheckError(): Other error\n";
		break;
	}
}

OpenGLFramebuffer* RenderingCore::PrivateInterface::CreateFramebuffer(GLuint width, GLuint height)
{
	return new OpenGLFramebuffer(width, height);
}

OpenGLScreenQuad* RenderingCore::PrivateInterface::CreateScreenQuad()
{
	return new OpenGLScreenQuad();
}

OpenGLEffect* RenderingCore::PrivateInterface::CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile)
{
	OpenGLEffect* effect = nullptr;

	if (!GLCreateEffectFromFile(vsfile, 0, 0, gsfile, fsfile, &effect))
		effect = nullptr;

	return effect;
}

OpenGLMesh* RenderingCore::PrivateInterface::CreateMesh(const char* file)
{
	OpenGLMesh* mesh = nullptr;

	if (!GLCreateMeshFromQM(file, &mesh))
		mesh = nullptr;

	return mesh;
}

OpenGLMesh* RenderingCore::PrivateInterface::CreateMesh(GLuint numvertices, GLuint numindices, GLuint flags, OpenGLVertexElement* decl)
{
	OpenGLMesh* mesh = nullptr;

	if (!GLCreateMesh(numvertices, numindices, flags, decl, &mesh))
		mesh = nullptr;

	return mesh;
}

// --- Core internal tasks ----------------------------------------------------

class BarrierTask : public IRenderingTask
{
private:
	std::promise<void> promise;

public:
	BarrierTask()
		: IRenderingTask(-3)
	{
	}

	void Execute(IRenderingContext* context) override
	{
		promise.set_value();
	}

	void Dispose() override
	{
	}

	inline std::future<void> GetFuture() {
		return promise.get_future();
	}
};

class UniverseCreatorTask : public IRenderingTask
{
	enum ContextAction
	{
		Create = 0,
		Delete = 1
	};

private:
	RenderingCore::PrivateInterface* privinterf;
	HDC hdc;
	int contextID;
	int contextaction;

public:
	UniverseCreatorTask(RenderingCore::PrivateInterface* interf, HDC dc)
		: IRenderingTask(-2)
	{
		privinterf		= interf;
		hdc				= dc;
		contextID		= -1;
		contextaction	= Create;
	}

	UniverseCreatorTask(RenderingCore::PrivateInterface* interf, int id)
		: IRenderingTask(0)
	{
		privinterf		= interf;
		hdc				= 0;
		contextID		= id;
		contextaction	= Delete;
	}

	void Dispose() override
	{
	}

	void Execute(IRenderingContext* context) override
	{
		if (contextaction == Create)
			contextID = privinterf->CreateContext(hdc);
		else if (contextaction == Delete)
			privinterf->DeleteContext(contextID);
	}

	inline int GetContextID() const	{ return contextID; }
};

// --- IRenderingTask impl ----------------------------------------------------

IRenderingTask::IRenderingTask(int universe)
{
	universeid = universe;
	disposing = false;
}

IRenderingTask::~IRenderingTask()
{
}

// --- RenderingCore impl -----------------------------------------------------

RenderingCore::RenderingCore()
{
	if (GLExtensions::GLVersion == 0)
		InitializeCoreProfile();

	privinterf = new PrivateInterface();
	thread = std::thread(&RenderingCore::THREAD_Run, this);
}

RenderingCore::~RenderingCore()
{
	delete privinterf;
}

int RenderingCore::CreateUniverse(HDC hdc)
{
	UniverseCreatorTask* creator = new UniverseCreatorTask(privinterf, hdc);

	AddTask(creator);
	Barrier();

	int id = creator->GetContextID();
	delete creator;

	return id;
}

void RenderingCore::DeleteUniverse(int id)
{
	UniverseCreatorTask* deleter = new UniverseCreatorTask(privinterf, id);

	AddTask(deleter);
	Barrier();

	delete deleter;
}

void RenderingCore::Barrier()
{
	BarrierTask* barrier = new BarrierTask();
	std::future<void>& future = barrier->GetFuture();

	AddTask(barrier);
	future.wait();
}

void RenderingCore::AddTask(IRenderingTask* task)
{
	tasks.Push(task);
}

void RenderingCore::Shutdown()
{
	tasks.Push(nullptr);
	thread.join();

	delete _inst;
	_inst = nullptr;
}

bool RenderingCore::InitializeCoreProfile()
{
	HWND dummy = CreateWindow(
		"AsylumSampleClass", "Dummy", WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		0, 0, 100, 100, 0, 0, GetModuleHandle(0), 0);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32, // color
		0, 0, 0, 0, 0, 0,
		0, // alpha
		0, 0, 0, 0, 0, 0,
		24, 8, 0, // depth, stencil, aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	HDC hdc = GetDC(dummy);
	V_RETURN(false, "InitializeCoreProfile(): Could not get device context", hdc);
	
	int pixelformat = ChoosePixelFormat(hdc, &pfd);
	V_RETURN(false, "InitializeCoreProfile(): Could not find suitable pixel format", pixelformat);

	BOOL success = SetPixelFormat(hdc, pixelformat, &pfd);
	V_RETURN(false, "InitializeCoreProfile(): Could not setup pixel format", success);

	HGLRC hrc = wglCreateContext(hdc);
	V_RETURN(false, "InitializeCoreProfile(): Context creation failed", hrc);

	success = wglMakeCurrent(hdc, hrc);
	V_RETURN(false, "InitializeCoreProfile(): Could not acquire context", success);

	GLExtensions::QueryFeatures(hdc);

	std::cout << "Vendor: " << GLExtensions::GLVendor << "\n";
	std::cout << "Renderer: " << GLExtensions::GLRenderer << "\n";
	std::cout << "Version: " << GLExtensions::GLAPIVersion << "\n\n";

	if (GLExtensions::GLVersion < GLExtensions::GL_3_3) {
		std::cout << "Device does not support OpenGL 3.3\n";
		return false;
	}

	success = wglMakeCurrent(hdc, NULL);
	V_RETURN(false, "InitializeCoreProfile(): Could not release dummy context", success);

	success = wglDeleteContext(hrc);
	V_RETURN(false, "InitializeCoreProfile(): Could not delete dummy context", success);

	ReleaseDC(dummy, hdc);
	DestroyWindow(dummy);

	return true;
}

void RenderingCore::THREAD_Run()
{
	while (true) {
		IRenderingTask* action = tasks.Pop();

		if (action == 0) {
			// exit call
			break;
		} else if (action->GetUniverseID() == -3) {
			// barrier task
			action->Execute(nullptr);
		} else if (action->GetUniverseID() == -2) {
			// context creator task
			action->Execute(privinterf);
		} else if (privinterf->ActivateContext(action->GetUniverseID())) {
			// rendering task
			action->Execute(privinterf);
		}

		if (action->IsMarkedForDispose()) {
			action->Dispose();

			delete action;
			action = nullptr;
		}
	}
}
