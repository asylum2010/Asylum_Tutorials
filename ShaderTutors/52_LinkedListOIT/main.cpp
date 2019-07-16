
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 52: Per-pixel linked list transparency"
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
OpenGLEffect*		init			= nullptr;
OpenGLEffect*		collect			= nullptr;
OpenGLEffect*		render			= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				headbuffer		= 0;
GLuint				nodebuffer		= 0;
GLuint				counterbuffer	= 0;
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
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/initheadpointers.frag", &init))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/renderfragments.frag", &render))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/collectfragments.vert", 0, 0, 0, "../../Media/ShadersGL/collectfragments.frag", &collect))
		return false;

	screenquad = new OpenGLScreenQuad();

	// create buffers
	size_t headsize = 16;	// start, count, pad, pad
	size_t nodesize = 16;	// color, depth, next, pad
	size_t numlists = screenwidth * screenheight;

	glGenBuffers(1, &headbuffer);
	glGenBuffers(1, &nodebuffer);
	glGenBuffers(1, &counterbuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numlists * headsize, 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numlists * 4 * nodesize, 0, GL_STATIC_DRAW);	// 120 MB @ 1080p

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

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
	delete init;
	delete collect;
	delete render;
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
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

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

	// STEP 1: initialize header pointer buffer
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, headbuffer);

	init->SetInt("screenWidth", screenwidth);
	init->Begin();
	{
		screenquad->Draw(false);
	}
	init->End();

	// STEP 2: collect transparent fragments into lists
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);

	GLuint* counter = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	*counter = 0;

	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodebuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterbuffer);

	collect->SetMatrix("matView", view);
	collect->SetMatrix("matProj", proj);
	collect->SetInt("screenWidth", screenwidth);

	collect->Begin();
	{
		for (int i = 0; i < ARRAY_SIZE(instances); ++i) {
			const ObjectInstance& obj = instances[i];

			collect->SetMatrix("matWorld", obj.transform);
			collect->SetVector("matColor", obj.color);
			collect->CommitChanges();

			(*obj.mesh)->Draw();
		}
	}
	collect->End();

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);

	// STEP 3: render
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);

	render->SetInt("screenWidth", screenwidth);

	render->Begin();
	{
		screenquad->Draw(false);
	}
	render->End();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	// render text
	glViewport(10, screenheight - 522, 512, 512);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
