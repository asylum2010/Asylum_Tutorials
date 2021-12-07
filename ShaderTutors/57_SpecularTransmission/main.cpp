
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <sstream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define MAX_SAMPLES			2048

// helper macros
#define TITLE				"Shader sample 57: Specular reflection and transmission"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample enums
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
	Math::Matrix	tounit;		// transforms ray to canonical basis
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

OpenGLEffect*		pathtracer		= nullptr;
OpenGLFramebuffer*	accumtargets[2]	= { nullptr };
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				sceneobjects	= 0;
GLuint				helptext		= 0;
GLuint				perfquery		= 0;
GLuint				queryresult		= 0;

BasicCamera			camera;

float				roughness		= 1.0f;
int					currtarget		= 0;
int					currsample		= 0;
bool				drawtext		= true;

static void CalcObjectParameters(SceneObject* data)
{
	Math::Matrix toworld;
	Math::Matrix tounit;
	Math::Color color;
	Math::Vector3 v0, v1, v2;
	Math::Vector3 t, b, n;
	float radius;
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

	// green/blue wall
	//color = Math::Color(0.1049f, 0.378f, 0.076f, 1);
	color = Math::Color(0.25f, 0.25f, 0.75f, 1);
	roughness = 1.0f;

	v0 = Math::Vector3(0, 0, 5.592f);
	v1 = Math::Vector3(0, 0, 0);
	v2 = Math::Vector3(0, 5.488f, 0);

	local_Calculate(0, BSDFTypeDiffuse, ObjectTypePlate);

	// red wall
	//color = Math::Color(0.57f, 0.042f, 0.044f, 1);
	color = Math::Color(0.75f, 0.25f, 0.25f, 1);

	v0 = Math::Vector3(5.496f, 0, 0);
	v1 = Math::Vector3(5.496f, 0, 5.592f);
	v2 = Math::Vector3(5.496f, 5.488f, 5.592f);

	local_Calculate(1, BSDFTypeDiffuse, ObjectTypePlate);
	
	// other walls
	color = Math::Color(0.885f, 0.698f, 0.666f, 1);
	//color = Math::Color(0.75f, 0.75f, 0.75f, 1);

	v0 = Math::Vector3(5.496f, 0, 5.592f);
	v1 = Math::Vector3(0, 0, 5.592f);
	v2 = Math::Vector3(0, 5.488f, 5.592f);

	local_Calculate(2, BSDFTypeDiffuse, ObjectTypePlate);

	v0 = Math::Vector3(5.496f, 5.488f, 5.592f);
	v1 = Math::Vector3(0, 5.488f, 5.592f);
	v2 = Math::Vector3(0, 5.488f, 0);

	local_Calculate(3, BSDFTypeDiffuse, ObjectTypePlate);

	v0 = Math::Vector3(5.496f, 0, 5.592f);
	v1 = Math::Vector3(0, 0, 5.592f);
	v2 = Math::Vector3(0, 0, 0);

	local_Calculate(4, BSDFTypeDiffuse, ObjectTypePlate);

	// silver sphere
	color = Math::Color(0.972f, 0.96f, 0.915f, 1);
	roughness = 0.1f;
	
	radius = 1.2f;
	v0 = Math::Vector3(4.1f, radius, 3.515f);

	local_Calculate(5, BSDFTypeConductor, ObjectTypeSphere); // BSDFTypeInsulator

	// glass sphere
	color = Math::Color(1, 1, 1, 1);
	
	radius = 1.2f;
	v0 = Math::Vector3(1.3f, radius, 1.685f);
	
	local_Calculate(6, BSDFTypeDielectric, ObjectTypeSphere); // |BSDFTypeSpecular
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
		data[i].textureID = -1;
	};

	center = Math::Vector3(2.78f, 5.488f, 2.795f);
	radius = 0.65f;
	radiance = Math::Color(12.3f, 12.3f, 12.3f, 0);

	local_Calculate(0);
}

static void UpdateText()
{
	std::stringstream ss;

	ss.precision(2);

	ss << "Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\n\nAdjust roughness with +/- (" << std::fixed << roughness << ")\n";
	ss << "Path tracer performance: " << (queryresult / 1000000) << " ms";

	GLRenderText(
		ss.str().c_str(),
		helptext, 512, 512);
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(SceneObject), nullptr, GL_STATIC_DRAW);

	SceneObject* data = (SceneObject*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	{
		CalcLightParameters(data);
		CalcObjectParameters(data + 1);
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
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/speculartransmission.frag", &pathtracer)) {
		MYERROR("Could not load shader");
		return false;
	}

	pathtracer->SetInt("prevIteration", 1);
	pathtracer->SetInt("albedoTextures", 2);

	screenquad = new OpenGLScreenQuad();

	glGenQueries(1, &perfquery);

	// setup camera
	Math::Vector3 eye(2.78f, 2.73f, -8);
	Math::Vector3 look(2.78f, 2.73f, 2.8f);
	Math::Vector3 v;

	float dist = Math::Vec3Distance(eye, look);

	Math::Vec3Normalize(v, eye - look);
	float angle = acosf(v.z);

	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(39.3077f));
	camera.SetPosition(look.x, look.y, look.z);
	camera.SetOrientation(angle, 0, 0);
	camera.SetDistance(dist);
	camera.SetClipPlanes(0.1f, 28);
	camera.SetIntertia(false);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);
	UpdateText();

	return true;
}

void UninitScene()
{
	delete pathtracer;
	delete accumtargets[0];
	delete accumtargets[1];
	delete screenquad;

	glDeleteQueries(1, &perfquery);

	GL_SAFE_DELETE_BUFFER(sceneobjects);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCodeH:
		drawtext = !drawtext;
		break;

	default:
		break;
	}

	if (key == KeyCodeAdd || key == KeyCodeSubtract) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneobjects);

		SceneObject* data = (SceneObject*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		{
			if (key == KeyCodeAdd)
				roughness = Math::Min(roughness + 0.05f, 1.0f);
			else if (key == KeyCodeSubtract)
				roughness = Math::Max(roughness - 0.05f, 0.05f);

			uint32_t temp = *((uint32_t*)&roughness);

			data[6].roughness = temp;
			data[7].roughness = temp;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		UpdateText();
		currsample = 0;
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
	static float lasttime = 0;
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

			pathtracer->SetMatrix("matViewProjInv", viewprojinv);
			pathtracer->SetVector("eyePos", eye);
			pathtracer->SetFloat("time", time);
			pathtracer->SetFloat("currSample", (float)currsample);

			pathtracer->SetInt("numObjects", 8);
			pathtracer->SetInt("numLights", 1);

			if (drawtext)
				glBeginQuery(GL_TIME_ELAPSED, perfquery);

			pathtracer->Begin();
			{
				screenquad->Draw(false);
			}
			pathtracer->End();

			if (drawtext) {
				glEndQuery(GL_TIME_ELAPSED);
				glGetQueryObjectuiv(perfquery, GL_QUERY_RESULT, &queryresult);
			}
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

	if (drawtext) {
		if (time - lasttime > 0.5f) {
			UpdateText();
			lasttime = time;
		}

		// render text
		uint32_t screenwidth = app->GetClientWidth();
		uint32_t screenheight = app->GetClientHeight();

		glViewport(10, screenheight - 522, 512, 512);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, helptext);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();

		// reset states
		glDisable(GL_BLEND);
		glViewport(0, 0, screenwidth, screenheight);
	}

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
