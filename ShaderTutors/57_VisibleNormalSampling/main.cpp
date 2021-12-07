
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
#define TITLE				"Shader sample 57: Sampling visible normals"
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
GLuint				textures		= 0;
GLuint				envmap			= 0;
GLuint				helptext		= 0;

BasicCamera			camera;

int					currtarget		= 0;
int					currsample		= 0;

static void CalcObjectParameters(SceneObject* data)
{
	Math::Matrix toworld;
	Math::Matrix tounit;
	Math::Color color;
	Math::Vector3 v0, v1, v2;
	Math::Vector3 t, b, n;
	float roughness, radius;
	float eta = 1.5f;

	auto local_Calculate = [&](int i, uint32_t bsdf, uint32_t type) {
		if (type == ObjectTypePlate) {
			t = v1 - v0;
			b = v2 - v1;

			Math::Vec3Cross(n, b, t);

			toworld = Math::Matrix(
				t.x, t.y, t.z, 0,
				n.x, n.y, n.z, 0,
				b.x, b.y, b.z, 0,
				v0.x, v0.y, v0.z, 1);

			Math::MatrixInverse(tounit, toworld);
		} else if (type == ObjectTypeSphere) {
			toworld = Math::Matrix(
				radius, 0, 0, 0,
				0, radius, 0, 0,
				0, 0, radius, 0,
				v0.x, v0.y, v0.z, 1);

			Math::MatrixInverse(tounit, toworld);
		}

		data[i].tounit = tounit;
		data[i].toworld = toworld;
		data[i].color = color;
		data[i].shapetype = type;
		data[i].textureID = -1;
		data[i].bsdftype = bsdf;
		data[i].roughness = *((uint32_t*)&roughness);
		data[i].eta = *((uint32_t*)&eta);
	};

#if 1
	// test materials
	color = Math::Color(1, 1, 1, 1);
	roughness = 0.005f;
	radius = 0.8f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, 3.6f);

	local_Calculate(0, BSDFTypeDielectric, ObjectTypeSphere);

	v0 = Math::Vector3(3.515f, -4.13f + radius, 1.2f);
	
	local_Calculate(1, BSDFTypeDiffuse, ObjectTypeSphere);

	v0 = Math::Vector3(3.515f, -4.13f + radius, -1.2f);

	color = Math::Color(0.972f, 0.96f, 0.915f, 1);
	local_Calculate(2, BSDFTypeConductor, ObjectTypeSphere);

	v0 = Math::Vector3(3.515f, -4.13f + radius, -3.6f);

	color = Math::Color::sRGBToLinear(0xff51acc9);
	local_Calculate(3, BSDFTypeInsulator, ObjectTypeSphere);
#elif 0
	// for article (reflection)
	radius = 0.8f;

	color = Math::Color(1.022f, 0.782f, 0.344f, 1);
	roughness = 0.05f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, 3.6f);
	local_Calculate(0, BSDFTypeConductor, ObjectTypeSphere);

	color = Math::Color(0.972f, 0.96f, 0.915f, 1);
	roughness = 0.25f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, 1.2f);
	local_Calculate(1, BSDFTypeConductor, ObjectTypeSphere);

	color = Math::Color(0.955f, 0.638f, 0.538f, 1);
	roughness = 0.5f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, -1.2f);
	local_Calculate(2, BSDFTypeConductor, ObjectTypeSphere);

	color = Math::Color(0.664f, 0.824f, 0.85f, 1);
	roughness = 0.75f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, -3.6f);
	local_Calculate(3, BSDFTypeConductor, ObjectTypeSphere);
#else
	// for article (refraction)
	radius = 0.8f;

	color = Math::Color(1, 1, 1, 1);
	roughness = 0.005f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, 3.6f);
	local_Calculate(0, BSDFTypeDielectric, ObjectTypeSphere);

	color = Math::Color(1, 0, 0, 1);
	roughness = 0.05f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, 1.2f);
	local_Calculate(1, BSDFTypeDielectric, ObjectTypeSphere);

	color = Math::Color(0, 1, 0, 1);
	roughness = 0.1f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, -1.2f);
	local_Calculate(2, BSDFTypeDielectric, ObjectTypeSphere);

	color = Math::Color(0, 0, 1, 1);
	roughness = 0.25f;

	v0 = Math::Vector3(3.515f, -4.13f + radius, -3.6f);
	local_Calculate(3, BSDFTypeDielectric, ObjectTypeSphere);
#endif

	// plates
	color = Math::Color(0.4f, 0.4f, 0.4f, 1.0f);
	roughness = 1.0f;

	v0 = Math::Vector3(-10, -4.14615f, -10);
	v1 = Math::Vector3(-10, -4.14615f, 10);
	v2 = Math::Vector3(10, -4.14615f, 10);

	local_Calculate(4, BSDFTypeDiffuse, ObjectTypePlate);
	data[4].textureID = 0;

	v0 = Math::Vector3(-2, -10, -10);
	v1 = Math::Vector3(-2, -10, 10);
	v2 = Math::Vector3(-2, 10, 10);

	local_Calculate(5, BSDFTypeDiffuse, ObjectTypePlate);
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * sizeof(SceneObject), nullptr, GL_STATIC_DRAW);

	SceneObject* data = (SceneObject*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	{
		CalcObjectParameters(data);
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
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/visiblenormals.frag", &integrator)) {
		MYERROR("Could not load shader");
		return false;
	}

	integrator->SetInt("prevIteration", 1);
	integrator->SetInt("albedoTextures", 2);
	integrator->SetInt("environmentMap", 3);

	screenquad = new OpenGLScreenQuad();

	// load albedo textures
	std::string files[] = {
		"../../Media/Textures/checker.jpg"
	};

	GLCreateTextureArrayFromFiles(files, ARRAY_SIZE(files), true, &textures);

	// load environment map
	if (!GLCreateCubeTextureFromDDS("../../Media/Textures/red_wall.dds", false, &envmap)) {
		MYERROR("Could not load environment mapr");
		return false;
	}

	// setup camera
	Math::Vector2 eye(2, 15);
	Math::Vector2 look(-2, 2.5f);
	Math::Vector2 v;

	float dist = Math::Vec2Distance(eye, look);

	Math::Vec2Normalize(v, eye - look);
	float angle = acosf(v.y);

	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(28));
	camera.SetPosition(look.y, look.x, 0);
	camera.SetOrientation(-Math::HALF_PI, angle, 0);
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
	GL_SAFE_DELETE_TEXTURE(textures);
	GL_SAFE_DELETE_TEXTURE(envmap);
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
			GLSetTexture(GL_TEXTURE2, GL_TEXTURE_2D_ARRAY, textures);
			GLSetTexture(GL_TEXTURE3, GL_TEXTURE_CUBE_MAP, envmap);

			integrator->SetMatrix("matViewProjInv", viewprojinv);
			integrator->SetVector("eyePos", eye);
			integrator->SetFloat("time", time);
			integrator->SetFloat("currSample", (float)currsample);
			integrator->SetInt("numObjects", 5);

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
