
#include "../Framework/win32window.h"
#include "../Framework/renderingcore.h"
#include "../Framework/drawingitem.h"

class OpenGLAddonTask : public IRenderingTask
{
	enum AddonAction
	{
		AddonActionSetup = 0,
		AddonActionRender
	};

private:
	Math::Color			clearcolor;
	Math::Color			meshcolor;
	OpenGLMesh*			mesh;
	OpenGLEffect*		effect;
	OpenGLFramebuffer*	rendertarget;	// external
	Math::Vector3		meshoffset;
	float				rendertime;
	int					action;
	bool				cleardepth;

	void Internal_Render(IRenderingContext* context);

	void Dispose() override;
	void Execute(IRenderingContext* context) override;

public:
	OpenGLAddonTask(int universe, OpenGLFramebuffer* target);

	void Setup();
	void Render(float time);

	inline void SetClearOptions(const Math::Color& color, bool depth) {
		clearcolor = color;
		cleardepth = depth;
	}

	inline void SetMeshColor(const Math::Color& color) {
		meshcolor = color;
	}

	inline void SetMeshOffset(float x, float y, float z) {
		meshoffset = Math::Vector3(x, y, z);
	}

	inline void SetRenderTarget(OpenGLFramebuffer* target) {
		rendertarget = target;
	}
};

class RenderTargetBlitTask : public IRenderingTask
{
private:
	OpenGLFramebuffer*	source;
	OpenGLFramebuffer*	target;
	uint32_t			blitflags;

	void Dispose() override
	{
	}

	void Execute(IRenderingContext* context) override
	{
		context->Blit(source, target, blitflags);
	}

public:
	enum BlitFlags
	{
		Color = GL_COLOR_BUFFER_BIT,
		Depth = GL_DEPTH_BUFFER_BIT
	};

	RenderTargetBlitTask(int universe, OpenGLFramebuffer* from, OpenGLFramebuffer* to, uint32_t flags)
		: IRenderingTask(universe)
	{
		source = from;
		target = to;
		blitflags = flags;
	}
};

// --- OpenGLAddonTask impl ---------------------------------------------------

OpenGLAddonTask::OpenGLAddonTask(int universe, OpenGLFramebuffer* target)
	: IRenderingTask(universe)
{
	mesh			= 0;
	effect			= 0;
	clearcolor		= Math::Color(1, 1, 1, 1);
	meshcolor		= Math::Color(1, 1, 1, 1);
	rendertarget	= target;
	action			= AddonActionSetup;
	rendertime		= 0;
	cleardepth		= true;

	meshoffset = Math::Vector3(0, 0, 0);
}

void OpenGLAddonTask::Dispose()
{
	// NOTE: runs on renderer thread
	SAFE_DELETE(mesh);
	SAFE_DELETE(effect);

	rendertarget = 0;
}

void OpenGLAddonTask::Execute(IRenderingContext* context)
{
	if (IsMarkedForDispose())
		return;

	// NOTE: runs on renderer thread
	if (action == AddonActionSetup) {
		if (mesh == nullptr)
			mesh = context->CreateMesh("../../Media/MeshesQM/teapot.qm");

		if (effect == nullptr)
			effect = context->CreateEffect("../../Media/ShadersGL/blinnphong.vert", 0, "../../Media/ShadersGL/blinnphong.frag");
	} else if (action == AddonActionRender) {
		Internal_Render(context);
	}
}

void OpenGLAddonTask::Internal_Render(IRenderingContext* context)
{
	// NOTE: runs on renderer thread
	if (rendertarget == nullptr)
		return;

	Math::Matrix world, view, proj;
	Math::Matrix viewproj, tmp;

	Math::Vector4 lightpos	= { 6, 3, 10, 1 };
	Math::Vector3 eye		= { 0, 0, 3 };
	Math::Vector3 look		= { 0, 0, 0 };
	Math::Vector3 up		= { 0, 1, 0 };

	Math::MatrixLookAtRH(view, eye, look, up);
	Math::MatrixPerspectiveFovRH(
		proj, Math::DegreesToRadians(60), 
		(float)rendertarget->GetWidth() / (float)rendertarget->GetHeight(), 0.1f, 100.0f);
	
	Math::MatrixMultiply(viewproj, view, proj);

	//calculate world matrix
	Math::MatrixIdentity(world);
	
	// offset with bb center
	world._41 = -0.108f;
	world._42 = -0.7875f;

	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(rendertime * 20.0f, 360.0f)), 1, 0, 0);
	Math::MatrixMultiply(world, world, tmp);

	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(fmodf(rendertime * 20.0f, 360.0f)), 0, 1, 0);
	Math::MatrixMultiply(world, world, tmp);

	world._41 += meshoffset[0];
	world._42 += meshoffset[1];
	world._43 += meshoffset[2];

	// setup states
	context->SetCullMode(GL_BACK);
	context->SetDepthTest(GL_TRUE);
	context->SetDepthFunc(GL_LEQUAL);

	// render
	effect->SetMatrix("matWorld", world);
	effect->SetMatrix("matViewProj", viewproj);

	effect->SetVector("matColor", &meshcolor.r);
	effect->SetVector("eyePos", eye);
	effect->SetVector("lightPos", lightpos);

	rendertarget->Set();
	{
		if (cleardepth)
			context->Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, clearcolor);
		else
			context->Clear(GL_COLOR_BUFFER_BIT, clearcolor);

		effect->Begin();
		{
			mesh->DrawSubset(0);
		}
		effect->End();
	}
	rendertarget->Unset();

	context->CheckError();
}

void OpenGLAddonTask::Setup()
{
	// NOTE: runs on any other thread
	action = AddonActionSetup;

	GetRenderingCore()->AddTask(this);
	GetRenderingCore()->Barrier();
}

void OpenGLAddonTask::Render(float time)
{
	// NOTE: runs on any other thread
	action = AddonActionRender;
	rendertime = time;

	GetRenderingCore()->AddTask(this);
}

// --- Main window functions --------------------------------------------------

OpenGLAddonTask* addonrenderer = nullptr;

void MainWindow_Created(Win32Window* window)
{
	DrawingItem* drawingitem = window->GetDrawingItem();
	const DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();

	if (addonrenderer == nullptr) {
		addonrenderer = new OpenGLAddonTask(drawingitem->GetOpenGLUniverseID(), bottomlayer.GetRenderTarget());
		addonrenderer->Setup();
	}
}

void MainWindow_Closing(Win32Window*)
{
	if (addonrenderer != nullptr) {
		addonrenderer->MarkForDispose();
		
		GetRenderingCore()->AddTask(addonrenderer);
		GetRenderingCore()->Barrier();

		addonrenderer = nullptr;
	}
}

void MainWindow_KeyPress(Win32Window* window, WPARAM wparam)
{
	switch (wparam) {
	case VK_ESCAPE:
		window->Close();
		break;

	default:
		break;
	}
}

void MainWindow_Render(Win32Window* window, float alpha, float elapsedtime)
{
	static float time = 0;

	DrawingItem* drawingitem = window->GetDrawingItem();

	if (addonrenderer != nullptr) {
		addonrenderer->SetClearOptions(Math::Color(0.0f, 0.125f, 0.3f, 1.0f), true);
		addonrenderer->Render(time);
	}

	time += elapsedtime;

	drawingitem->RecomposeLayers();
}

// --- Window 3 functions -----------------------------------------------------

OpenGLAddonTask* window3renderer = nullptr;

void Window3_Created(Win32Window* window)
{
	DrawingItem* drawingitem = window->GetDrawingItem();
	const DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();

	if (window3renderer == nullptr) {
		window3renderer = new OpenGLAddonTask(drawingitem->GetOpenGLUniverseID(), bottomlayer.GetRenderTarget());
		window3renderer->Setup();
	}
}

void Window3_Closing(Win32Window*)
{
	if (window3renderer != nullptr) {
		window3renderer->MarkForDispose();
		
		GetRenderingCore()->AddTask(window3renderer);
		GetRenderingCore()->Barrier();

		window3renderer = nullptr;
	}
}

void Window3_Render(Win32Window* window, float alpha, float elapsedtime)
{
	static float time = 0;

	DrawingItem* drawingitem = window->GetDrawingItem();

	if (window3renderer != nullptr) {
		OpenGLFramebuffer* bottomlayer = drawingitem->GetBottomLayer().GetRenderTarget();
		OpenGLFramebuffer* feedbacklayer = drawingitem->GetFeedbackLayer().GetRenderTarget();

		window3renderer->SetRenderTarget(bottomlayer);
		window3renderer->SetClearOptions(Math::Color(0.0f, 0.125f, 0.3f, 1.0f), true);
		window3renderer->SetMeshColor(Math::Color(1, 1, 1, 1));
		window3renderer->SetMeshOffset(-0.75f, 0, 0);
		window3renderer->Render(time);

		RenderTargetBlitTask* blitter = new RenderTargetBlitTask(
			drawingitem->GetOpenGLUniverseID(), bottomlayer, feedbacklayer, RenderTargetBlitTask::Depth);

		blitter->MarkForDispose();

		GetRenderingCore()->AddTask(blitter);
		GetRenderingCore()->Barrier();

		window3renderer->SetRenderTarget(feedbacklayer);
		window3renderer->SetClearOptions(Math::Color(0, 0, 0, 0), false);
		window3renderer->SetMeshColor(Math::Color(0, 1, 0, 1));
		window3renderer->SetMeshOffset(0.75f, 0, 0);
		window3renderer->Render(-time);
	}

	time += elapsedtime;

	drawingitem->RecomposeLayers();
}
