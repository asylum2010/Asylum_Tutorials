
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 51: Weighted Blended Order-Independent Trasparency"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct ObjectInstance
{
	Math::Matrix	transform;
	Math::Color		color;
	Math::Vector3	position;
	Math::Vector3	scale;
	OpenGLMesh**	mesh;
	float			angle;
};

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			box				= nullptr;
OpenGLMesh*			buddha			= nullptr;
OpenGLMesh*			dragon			= nullptr;
OpenGLFramebuffer*	accumulator		= nullptr;
OpenGLFramebuffer*	resolvetarget	= nullptr;
OpenGLEffect*		accumeffect		= nullptr;
OpenGLEffect*		composeeffect	= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				helptext		= 0;

BasicCamera			camera;
Math::AABox			scenebox;

// object instances
ObjectInstance instances[] =
{
	//		color							position				scaling				mesh		angle
	{ {}, Math::Color(1, 1, 0, 0.75f),	{ 0, -0.35f, 0 },		{ 15, 0.5f, 15 },		&box,		0,								},

	{ {}, Math::Color(1, 0, 1, 0.5f),	{ -1, -0.1f, 2.5f },	{ 0.3f, 0.3f, 0.3f },	&dragon,	-Math::PI / 8,					},
	{ {}, Math::Color(0, 1, 1, 0.5f),	{ 2.5f, -0.1f, 0 },		{ 0.3f, 0.3f, 0.3f },	&dragon,	Math::PI / -2 + Math::PI / -6,	},
	{ {}, Math::Color(1, 0, 0, 0.5f),	{ -2, -0.1f, -2 },		{ 0.3f, 0.3f, 0.3f },	&dragon,	Math::PI / -4,					},

	{ {}, Math::Color(0, 1, 0, 0.5f),	{ 0, -1.15f, 0 },		{ 20, 20, 20 },			&buddha,	Math::PI,						},
};

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// load objects
	if (!GLCreateMeshFromQM("../../Media/MeshesQM/box.qm", &box)) {
		MYERROR("Could not load box");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/dragon.qm", &dragon)) {
		MYERROR("Could not load dragon");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/happy1.qm", &buddha)) {
		MYERROR("Could not load buddha");
		return false;
	}

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/weighted_blended.vert", 0, 0, 0, "../../Media/ShadersGL/weighted_blended.frag", &accumeffect))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/weighted_compose.frag", &composeeffect))
		return false;

	composeeffect->SetInt("sumColors", 0);
	composeeffect->SetInt("sumWeights", 1);

	screenquad = new OpenGLScreenQuad();

	// create framebuffers
	accumulator = new OpenGLFramebuffer(screenwidth, screenheight);

	accumulator->AttachRenderbuffer(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, 4);
	accumulator->AttachRenderbuffer(GL_COLOR_ATTACHMENT1, GLFMT_R16F, 4);

	if (!accumulator->Validate())
		return false;

	resolvetarget = new OpenGLFramebuffer(screenwidth, screenheight);

	resolvetarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);
	resolvetarget->AttachTexture(GL_COLOR_ATTACHMENT1, GLFMT_R16F, GL_NEAREST);

	if (!resolvetarget->Validate())
		return false;

	// calculate scene bounding box
	Math::AABox tmpbox;
	Math::Matrix tmpmat;

	for (int i = 0; i < ARRAY_SIZE(instances); ++i) {
		ObjectInstance& obj = instances[i];

		// scaling * rotation * translation
		Math::MatrixScaling(tmpmat, obj.scale[0], obj.scale[1], obj.scale[2]);
		Math::MatrixRotationAxis(obj.transform, obj.angle, 0, 1, 0);
		Math::MatrixMultiply(obj.transform, tmpmat, obj.transform);

		Math::MatrixTranslation(tmpmat, obj.position[0], obj.position[1], obj.position[2]);
		Math::MatrixMultiply(obj.transform, obj.transform, tmpmat);

		tmpbox = (*obj.mesh)->GetBoundingBox();
		tmpbox.TransformAxisAligned(obj.transform);

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(8);
	camera.SetOrientation(-0.25f, 0.7f, 0);
	camera.SetPosition(0, 0.3f, 0);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera",
		helptext, 512, 512);

	return true;
}

void UninitScene()
{
	delete box;
	delete dragon;
	delete buddha;
	delete accumulator;
	delete resolvetarget;
	delete accumeffect;
	delete composeeffect;
	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		camera.OrbitRight(Math::DegreesToRadians(dx));
		camera.OrbitUp(Math::DegreesToRadians(dy));
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix world, view, proj;
	Math::Matrix viewproj;
	Math::Vector3 eye, look;
	Math::Vector2 clipplanes;

	camera.Animate(alpha);

	camera.GetPosition(look);
	camera.GetEyePosition(eye);
	camera.GetViewMatrix(view);

	Math::FitToBoxPerspective(clipplanes, eye, look - eye, scenebox);
	
	camera.SetClipPlanes(clipplanes[0], clipplanes[1]);
	camera.GetProjectionMatrix(proj);
	
	Math::MatrixMultiply(viewproj, view, proj);

	// accumulate
	accumulator->Set();
	{
		float black[] = { 0, 0, 0, 0 };
		float white[] = { 1, 1, 1, 1 };

		glClearBufferfv(GL_COLOR, 0, black);
		glClearBufferfv(GL_COLOR, 1, white);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

		accumeffect->SetMatrix("matViewProj", viewproj);
		accumeffect->Begin();
		{
			for (int i = 0; i < ARRAY_SIZE(instances); ++i) {
				const ObjectInstance& obj = instances[i];

				accumeffect->SetMatrix("matWorld", obj.transform);
				accumeffect->SetVector("matColor", obj.color);
				accumeffect->CommitChanges();

				(*obj.mesh)->Draw();
			}
		}
		accumeffect->End();
	}
	accumulator->Unset();

	accumulator->Resolve(resolvetarget, GL_COLOR_BUFFER_BIT);

	// compose
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, resolvetarget->GetColorAttachment(0));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, resolvetarget->GetColorAttachment(1));

	glBlendFuncSeparate(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ZERO, GL_ONE);

	Math::MatrixIdentity(world);
	composeeffect->SetMatrix("matTexture", world);

	composeeffect->Begin();
	{
		screenquad->Draw(false);
	}
	composeeffect->End();

	glActiveTexture(GL_TEXTURE0);

	// render text
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glViewport(10, screenheight - 522, 512, 512);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
	Math::MatrixReflect(world, xzplane);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, helptext);

	screenquad->SetTextureMatrix(world);
	screenquad->Draw();

	// reset states
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glViewport(0, 0, screenwidth, screenheight);

	// check errors
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		std::cout << "Error\n";

	app->Present();
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGL)) {
		delete app;
		return 1;
	}

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseScrollCallback = MouseScroll;

	app->Run();
	delete app;

	return 0;
}
