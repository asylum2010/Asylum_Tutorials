
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define MAX_SAMPLES			1024

// helper macros
#define TITLE				"Shader sample 57: Multiple importance sampling"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

enum ObjectType
{
	ObjectTypeEmitter = 1,
	ObjectTypePlate = 2,
	ObjectTypeSphere = 4
};

enum BSDFType
{
	BSDFTypeDiffuse = 0,
	BSDFTypeSpecular = 1,	// Dirac delta (when roughness < 0.1)
	BSDFTypeInsulator = 2,
	BSDFTypeConductor = 4,	// metals
	BSDFTypeDielectric = 8,	// glass, diamond, etc.
};

// sample structures
struct SceneObject
{
	Math::Matrix	tounit;	// transforms ray to canonical basis
	Math::Matrix	toworld;
	uint32_t		shapetype;
	uint32_t		padding[3];

	Math::Color		color;
	uint32_t		textureID;
	uint32_t		bsdftype;
	uint32_t		roughness;
	uint32_t		eta;
};

static_assert(sizeof(SceneObject) % 16 == 0, "SceneObject must be 16 byte aligned");

// sample variables
Application*		app				= nullptr;

OpenGLEffect*		integrator		= nullptr;
OpenGLFramebuffer*	accumtargets[2]	= { nullptr };
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				sceneobjects	= 0;
GLuint				helptext		= 0;

BasicCamera			camera;

int					currtarget		= 0;
int					currsample		= 0;

static void CalcObjectParameters(SceneObject* data)
{
	Math::Matrix plate;
	Math::Matrix tounit;
	Math::Color color;
	Math::Vector3 v0, v1, v2;
	Math::Vector3 t, b, n;
	float roughness;
	float eta = 2.0f;

	auto local_Calculate = [&](int i) {
		t = v2 - v1;
		b = v1 - v0;

		Math::Vec3Cross(n, b, t);

		plate = Math::Matrix(
			t.x, t.y, t.z, 0,
			n.x, n.y, n.z, 0,
			b.x, b.y, b.z, 0,
			v0.x, v0.y, v0.z, 1);

		Math::MatrixInverse(tounit, plate);

		data[i].tounit = tounit;
		data[i].toworld = plate;
		data[i].color = color;
		data[i].shapetype = ObjectTypePlate;
		data[i].bsdftype = BSDFTypeInsulator;
		data[i].textureID = -1;
		data[i].roughness = *((uint32_t*)&roughness);
		data[i].eta = *((uint32_t*)&eta);
	};

	color = Math::Color(0.07f, 0.09f, 0.13f, 1.0f);
	roughness = 0.005f;

	v0 = Math::Vector3(4, -2.70651f, 0.25609f);
	v1 = Math::Vector3(4, -2.08375f, -0.526323f);
	v2 = Math::Vector3(-4, -2.08375f, -0.526323f);

	local_Calculate(0);

	roughness = 0.02f;

	v0 = Math::Vector3(4, -3.28825f, 1.36972f);
	v1 = Math::Vector3(4, -2.83856f, 0.476536f);
	v2 = Math::Vector3(-4, -2.83856f, 0.476536f);

	local_Calculate(1);

	roughness = 0.05f;

	v0 = Math::Vector3(4, -3.73096f, 2.70046f);
	v1 = Math::Vector3(4, -3.43378f, 1.74564f);
	v2 = Math::Vector3(-4, -3.43378f, 1.74564f);

	local_Calculate(2);

	roughness = 0.1f;

	v0 = Math::Vector3(4, -3.99615f, 4.0667f);
	v1 = Math::Vector3(4, -3.82069f, 3.08221f);
	v2 = Math::Vector3(-4, -3.82069f, 3.08221f);

	local_Calculate(3);

	color = Math::Color(0.4f, 0.4f, 0.4f, 1.0f);
	roughness = 1.0f;

	v0 = Math::Vector3(-10, -4.14615f, -10);
	v1 = Math::Vector3(-10, -4.14615f, 10);
	v2 = Math::Vector3(10, -4.14615f, 10);

	local_Calculate(4);

	v0 = Math::Vector3(-10, -10, -2);
	v1 = Math::Vector3(10, -10, -2);
	v2 = Math::Vector3(10, 10, -2);

	local_Calculate(5);
}

static void CalcLightParameters(SceneObject* data)
{
	Math::Matrix sphere;
	Math::Matrix tounit;
	Math::Vector3 center;
	Math::Color radiance;
	float radius;

	auto local_Calculate = [&](int i) {
		sphere = Math::Matrix(
			radius, 0, 0, 0,
			0, radius, 0, 0,
			0, 0, radius, 0,
			center.x, center.y, center.z, 1);

		Math::MatrixInverse(tounit, sphere);

		data[i].tounit = tounit;
		data[i].toworld = sphere;
		data[i].color = radiance;
		data[i].shapetype = ObjectTypeEmitter|ObjectTypeSphere;
	};

	center = Math::Vector3(10, 10, 4);
	radius = 0.5f;
	radiance = Math::Color(800, 800, 800, 0);

	local_Calculate(0);

	center = Math::Vector3(-1.25f, 0, 0);
	radius = 0.1f;
	radiance = Math::Color(100, 100, 100, 0);

	local_Calculate(1);

	center = Math::Vector3(-3.75f, 0, 0);
	radius = 0.0333f;
	radiance = Math::Color(901.803f, 901.803f, 901.803f, 0);

	local_Calculate(2);

	center = Math::Vector3(1.25f, 0, 0);
	radius = 0.3f;
	radiance = Math::Color(11.1111f, 11.1111f, 11.1111f, 0);

	local_Calculate(3);

	center = Math::Vector3(3.75f, 0, 0);
	radius = 0.9f;
	radiance = Math::Color(1.23457f, 1.23457f, 1.23457f, 0);

	local_Calculate(4);
}

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// create scene
	glGenBuffers(1, &sceneobjects);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneobjects);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 11 * sizeof(SceneObject), nullptr, GL_STATIC_DRAW);

	SceneObject* data = (SceneObject*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	{
		CalcLightParameters(data);
		CalcObjectParameters(data + 5);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// create render targets
	accumtargets[0] = new OpenGLFramebuffer(screenwidth, screenheight);
	accumtargets[1] = new OpenGLFramebuffer(screenwidth, screenheight);

	accumtargets[0]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A32B32G32R32F, GL_NEAREST);
	accumtargets[1]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A32B32G32R32F, GL_NEAREST);

	GL_ASSERT(accumtargets[0]->Validate());
	GL_ASSERT(accumtargets[1]->Validate());

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/multipleimportance.frag", &integrator)) {
		MYERROR("Could not load shader");
		return false;
	}

	integrator->SetInt("prevIteration", 1);
	integrator->SetInt("albedoTextures", 2);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	Math::Vector2 eye(2, 15);
	Math::Vector2 look(-2, 2.5f);
	Math::Vector2 v;

	float dist = Math::Vec2Distance(eye, look);

	Math::Vec2Normalize(v, eye - look);
	float angle = acosf(v.y);

	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(28));
	camera.SetPosition(0, look.x, look.y);
	camera.SetOrientation(0, angle, 0);
	camera.SetDistance(dist);
	camera.SetClipPlanes(0.1f, 20);
	camera.SetIntertia(false);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera",
		helptext, 512, 512);

	return true;
}

void UninitScene()
{
	delete integrator;
	delete accumtargets[0];
	delete accumtargets[1];
	delete screenquad;

	GL_SAFE_DELETE_BUFFER(sceneobjects);
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

		currsample = 0;
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);

		currsample = 0;
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
	currsample = 0;
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	Math::Matrix world, view, proj;
	Math::Matrix viewproj, viewprojinv;
	Math::Vector3 eye;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixInverse(viewprojinv, viewproj);

	// render
	if (currsample == 0) {
		accumtargets[currtarget]->Set();
		glClear(GL_COLOR_BUFFER_BIT);
		accumtargets[currtarget]->Unset();
	}

	if (currsample < MAX_SAMPLES) {
		currtarget = 1 - currtarget;
		++currsample;

		accumtargets[currtarget]->Set();
		{
			glClear(GL_COLOR_BUFFER_BIT);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sceneobjects);

			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, accumtargets[1 - currtarget]->GetColorAttachment(0));

			integrator->SetMatrix("matViewProjInv", viewprojinv);
			integrator->SetVector("eyePos", eye);
			integrator->SetFloat("time", time);
			integrator->SetFloat("currSample", (float)currsample);
			integrator->SetInt("numObjects", 11);
			integrator->SetInt("numLights", 5);

			integrator->Begin();
			{
				screenquad->Draw(false);
			}
			integrator->End();
		}
		accumtargets[currtarget]->Unset();
	}

	// present
	Math::MatrixIdentity(world);
	screenquad->SetTextureMatrix(world);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glClear(GL_COLOR_BUFFER_BIT);

	GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, accumtargets[currtarget]->GetColorAttachment(0));
	screenquad->Draw();

	time += elapsedtime;

	// render text
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glViewport(10, screenheight - 522, 512, 512);
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
	Math::MatrixReflect(world, xzplane);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, helptext);

	screenquad->SetTextureMatrix(world);
	screenquad->Draw();

	// reset states
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
