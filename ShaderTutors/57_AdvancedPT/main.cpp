
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <sstream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\gl4bvh.h"
#include "..\Common\spectatorcamera.h"

// render control
const bool BIDIRECTIONAL			= false;
const bool ENABLE_TWOSIDED			= true;		// for veach_bidir and similar scenes
const bool NEXT_EVENT_ESTIMATION	= true;		// severely slows down complex scenes
const bool WHITE_FURNACE_TEST		= false;

//const char* SCENE_FILE		= "livingroom3/livingroom3";
//const char* SCENE_FILE		= "livingroom2/livingroom2";
//const char* SCENE_FILE		= "kitchen/kitchen";
//const char* SCENE_FILE		= "bathroom/bathroom";
//const char* SCENE_FILE		= "bedroom/bedroom";
const char* SCENE_FILE			= "veach-bidir/veach-bidir";
//const char* SCENE_FILE		= "bathroom2/bathroom2";

// helper macros
#define TITLE				"Shader sample 57: Advanced Monte Carlo methods"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample enums
enum BSDFType
{
	BSDFTypeDiffuse = 0,
	BSDFTypeSpecular = 1,		// Dirac delta (when roughness < 0.1)
	BSDFTypeInsulator = 2,
	BSDFTypeConductor = 4,		// metals
	BSDFTypeDielectric = 8,		// glass, diamond, etc.
	BSDFTypeTransparent = 16	// alpha-blended diffuse
};

enum ShapeType
{
	ShapeTypePlate = 2,
	ShapeTypeSphere = 4
};

// sample structures
struct BSDFInfo
{
	Math::Color	color;
	uint32_t	textureID;
	uint32_t	bsdftype;
	uint32_t	roughness;
	uint32_t	eta;
};

struct AreaLight
{
	Math::Matrix	tounit;
	Math::Matrix	toworld;
	Math::Vector3	luminance;
	uint32_t		shapetype;
};

static_assert(sizeof(BSDFInfo) == 32, "sizeof(BSDFInfo) must be 32 B");
static_assert(sizeof(AreaLight) == 144, "sizeof(AreaLight) must be 144 B");

// sample variables
Application*		app				= nullptr;

OpenGLScreenQuad*	screenquad		= nullptr;
OpenGLMesh*			model			= nullptr;
OpenGLMesh*			debugmesh		= nullptr;
OpenGLEffect*		bdpathtracer	= nullptr;
OpenGLEffect*		debugdraw		= nullptr;
OpenGLBVH*			accelstructure	= nullptr;

SpectatorCamera		camera;
GLuint				lightbuffer		= 0;
GLuint				materialbuffer	= 0;

GLuint				textures		= 0;
GLuint				accumtarget		= 0;
GLuint				countertarget	= 0;
GLuint				rendertarget	= 0;

GLuint				helptext		= 0;
GLuint				perfquery		= 0;
GLuint				queryresult		= 0;

uint32_t			numlights		= 0;
uint32_t			nummaterials	= 0;

uint32_t			screenwidth;
uint32_t			screenheight;
bool				drawtext		= true;

static bool LoadScene(const std::string& filepath)
{
	std::string path = "../../Media/MeshesQM/" + filepath + ".qm";
	std::string name;
	FILE* infile = nullptr;

	if (!GLCreateMeshFromQM(path.c_str(), &model, GLMESH_32BIT))
		return false;

	path = "../../Media/MeshesQM/" + filepath + ".info";
	fopen_s(&infile, path.c_str(), "rb");

	if (!infile)
		return false;

	Math::GetFile(name, filepath);

	// load scene info
	AreaLight*		lights			= nullptr;
	BSDFInfo*		materials		= nullptr;
	std::string*	texturenames	= nullptr;
	uint32_t*		overridetable	= nullptr;
	Math::Vector3	eye;
	Math::Vector3	orient;
	float			fov;

	uint32_t		numenvmaps		= 0;
	uint32_t		numtextures		= 0;
	uint32_t		nummaterials	= 0;
	uint32_t		numoverrides	= 0;
	uint32_t		length;

	fread(&numenvmaps, sizeof(uint32_t), 1, infile);
	fread(&numlights, sizeof(uint32_t), 1, infile);
	fread(&numtextures, sizeof(uint32_t), 1, infile);
	fread(&nummaterials, sizeof(uint32_t), 1, infile);
	fread(&numoverrides, sizeof(uint32_t), 1, infile);
	
	assert(numoverrides >= model->GetNumSubsets());
	overridetable = new uint32_t[numoverrides];

	//if (numenvmaps > 0) {
	// TODO:
	//}

	if (numlights > 0) {
		lights = new AreaLight[numlights + 1];
		fread(lights, sizeof(AreaLight), numlights, infile);

		// dummy light
		lights[numlights].luminance = Math::Vector3(0, 0, 0);
		lights[numlights].shapetype = ShapeTypeSphere;

		Math::MatrixIdentity(lights[numlights].tounit);
		Math::MatrixIdentity(lights[numlights].toworld);
	}

	if (numtextures > 0) {
		texturenames = new std::string[numtextures];

		for (uint32_t i = 0; i < numtextures; ++i) {
			fread(&length, sizeof(uint32_t), 1, infile);
			texturenames[i].resize(length - 1);

			fread(&texturenames[i][0], 1, length, infile);
			texturenames[i] = "../../Media/MeshesQM/" + name + "/" + texturenames[i];
		}
	}

	if (nummaterials > 0) {
		materials = new BSDFInfo[nummaterials];
		fread(materials, sizeof(BSDFInfo), nummaterials, infile);
	}

	fread(&fov, sizeof(float), 1, infile);
	fread(&eye, sizeof(Math::Vector3), 1, infile);
	fread(&orient, sizeof(Math::Vector3), 1, infile);
	fread(overridetable, sizeof(uint32_t), numoverrides, infile);

	fclose(infile);

	if (WHITE_FURNACE_TEST) {
		for (uint32_t i = 0; i < nummaterials; ++i) {
			BSDFInfo& mat = materials[i];
			
			mat.bsdftype = BSDFTypeDiffuse;
			mat.color = Math::Color(1, 1, 1, 1);
			mat.textureID = -1;
		}

		for (uint32_t i = 0; i < numlights; ++i) {
			AreaLight& light = lights[i];

			light.luminance = Math::Vector3(1, 1, 1);
		}
	}

	if (numtextures > 0) {
		// load textures
		if (!GLCreateTextureArrayFromFiles(texturenames, numtextures, true, &textures)) {
			return false;
		}
	}

	if (numlights > 0) {
		// fill light buffer
		glGenBuffers(1, &lightbuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);

		glBufferStorage(GL_SHADER_STORAGE_BUFFER, (numlights + 1) * sizeof(AreaLight), lights, 0);
	}

	if (nummaterials > 0) {
		// fill material buffer
		glGenBuffers(1, &materialbuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialbuffer);

		glBufferStorage(GL_SHADER_STORAGE_BUFFER, nummaterials * sizeof(BSDFInfo), materials, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	camera.SetFov(fov);
	camera.SetEyePosition(eye.x, eye.y, eye.z);
	camera.SetOrientation(orient.x, orient.y, orient.z);

	delete[] texturenames;
	delete[] lights;
	delete[] materials;

	// build BVH
	path = "../../Media/Cache/" + name + ".bvh";

	accelstructure = new OpenGLBVH();
	accelstructure->Build(model, overridetable, path.c_str());

	delete[] overridetable;

	return true;
}

static void UpdateText()
{
	std::stringstream ss;

	ss.precision(2);
	ss << "Use WASD and mouse left to navigate\n\n";
	ss << "Path tracer performance: " << (queryresult / 1000000) << " ms";

	GLRenderText(
		ss.str().c_str(),
		helptext, 512, 512);
}

bool InitScene()
{
	std::stringstream defines;

	screenwidth = app->GetClientWidth();
	screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_TEST);

	if (!LoadScene(SCENE_FILE)) {
		return false;
	}

	if (ENABLE_TWOSIDED)
		defines << "#define ENABLE_TWOSIDED\n";

	if (NEXT_EVENT_ESTIMATION)
		defines << "#define NEXT_EVENT_ESTIMATION\n";

	// load shaders
	defines << "#define NUM_LIGHTS	" << numlights << "\n#define NUM_MATERIALS	" << nummaterials << "\n";

	if (BIDIRECTIONAL)
		GLCreateComputeProgramFromFile("../../Media/ShadersGL/bdpt.comp", &bdpathtracer, defines.str().c_str());
	else
		GLCreateComputeProgramFromFile("../../Media/ShadersGL/udpt.comp", &bdpathtracer, defines.str().c_str());

	bdpathtracer->Introspect();

	bdpathtracer->SetInt("accumTarget", 0);
	bdpathtracer->SetInt("counterTarget", 1);
	bdpathtracer->SetInt("renderTarget", 2);
	bdpathtracer->SetInt("albedoTextures", 3);

	GLCreateEffectFromFile("../../Media/ShadersGL/renderbvh.vert", 0, 0, 0, "../../Media/ShadersGL/renderbvh.frag", &debugdraw);

	// create render targets
	glGenTextures(1, &accumtarget);
	glGenTextures(1, &countertarget);
	glGenTextures(1, &rendertarget);

	glBindTexture(GL_TEXTURE_2D, accumtarget);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenwidth, screenheight, 0, GL_RGBA, GL_FLOAT, 0);

	glBindTexture(GL_TEXTURE_2D, countertarget);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, screenwidth, screenheight, 0, GL_RG, GL_FLOAT, 0);

	glBindTexture(GL_TEXTURE_2D, rendertarget);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenwidth, screenheight, 0, GL_RGBA, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	float zeroes[] = { 0, 0, 0, 1 };

	glClearTexImage(accumtarget, 0, GL_RGBA, GL_FLOAT, zeroes);
	glClearTexImage(countertarget, 0, GL_RG, GL_FLOAT, zeroes);

	// other
	Math::Vector3* vdata = nullptr;
	uint16_t* idata = nullptr;

	OpenGLVertexElement decl[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	uint16_t wireindices[] = {
		0, 1, 1, 3, 3, 2,
		2, 0, 0, 4, 4, 6,
		6, 2, 4, 5, 5, 7,
		7, 6, 7, 3, 1, 5
	};

	GLCreateMesh(8, 24, 0, decl, &debugmesh);

	debugmesh->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	debugmesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		vdata[0] = Math::Vector3(-0.5f, -0.5f, -0.5f);
		vdata[1] = Math::Vector3(-0.5f, -0.5f, 0.5f);
		vdata[2] = Math::Vector3(-0.5f, 0.5f, -0.5f);
		vdata[3] = Math::Vector3(-0.5f, 0.5f, 0.5f);

		vdata[4] = Math::Vector3(0.5f, -0.5f, -0.5f);
		vdata[5] = Math::Vector3(0.5f, -0.5f, 0.5f);
		vdata[6] = Math::Vector3(0.5f, 0.5f, -0.5f);
		vdata[7] = Math::Vector3(0.5f, 0.5f, 0.5f);

		memcpy(idata, wireindices, 24 * sizeof(uint16_t));
	}
	debugmesh->UnlockIndexBuffer();
	debugmesh->UnlockVertexBuffer();

	debugmesh->GetAttributeTable()[0].PrimitiveType = GL_LINES;

	screenquad = new OpenGLScreenQuad();
	glGenQueries(1, &perfquery);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetIntertia(false);

	// NOTE: test against pbrt-v3
	//camera.SetFov(Math::DegreesToRadians(34.1565475f));
	//camera.SetClipPlanes(0.01f, 50.0f);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);
	UpdateText();

	return true;
}

void UninitScene()
{
	delete accelstructure;
	delete model;
	delete debugmesh;
	delete bdpathtracer;
	delete debugdraw;
	delete screenquad;
	
	glDeleteQueries(1, &perfquery);

	GL_SAFE_DELETE_TEXTURE(accumtarget);
	GL_SAFE_DELETE_TEXTURE(countertarget);
	GL_SAFE_DELETE_TEXTURE(rendertarget);

	GL_SAFE_DELETE_BUFFER(lightbuffer);
	GL_SAFE_DELETE_BUFFER(materialbuffer);

	GL_SAFE_DELETE_TEXTURE(textures);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyDown(KeyCode key)
{
	camera.OnKeyDown(key);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	default:
		camera.OnKeyUp(key);
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	camera.OnMouseMove(dx, dy);
}

void MouseDown(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseDown(button);
}

void MouseUp(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseUp(button);
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
	Math::Vector3 eye, forward;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	forward.x = -view._13;
	forward.y = -view._23;
	forward.z = -view._33;

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixInverse(viewprojinv, viewproj);

	if (!camera.IsAnimationFinished()) {
		float zeroes[] = { 0, 0, 0, 1 };

		glClearTexImage(accumtarget, 0, GL_RGBA, GL_FLOAT, zeroes);
		glClearTexImage(countertarget, 0, GL_RG, GL_FLOAT, zeroes);
	}

	float cameraarea = Math::ImagePlaneArea(proj);

	// render
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();
	Math::Vector4 texelsize(1.0f / screenwidth, 1.0f / screenheight, 0, 0);

	bdpathtracer->SetMatrix("matViewProj", viewproj);
	bdpathtracer->SetMatrix("matViewProjInv", viewprojinv);
	bdpathtracer->SetVector("eyePos", &eye.x);
	bdpathtracer->SetVector("camForward", &forward.x);
	bdpathtracer->SetVector("texelSize", texelsize);
	bdpathtracer->SetFloat("time", time);
	bdpathtracer->SetFloat("camArea", cameraarea);

	if (drawtext)
		glBeginQuery(GL_TIME_ELAPSED, perfquery);

	bdpathtracer->Begin();
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, model->GetVertexBuffer());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, model->GetIndexBuffer());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, accelstructure->GetHierarchy());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, accelstructure->GetTriangleIDs());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, lightbuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, materialbuffer);

		glBindImageTexture(0, accumtarget, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, countertarget, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
		glBindImageTexture(2, rendertarget, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		GLSetTexture(GL_TEXTURE3, GL_TEXTURE_2D_ARRAY, textures);

		glDispatchCompute(screenwidth / 16, screenheight / 16, 1);
	}
	bdpathtracer->End();

	if (drawtext) {
		glEndQuery(GL_TIME_ELAPSED);
		glGetQueryObjectuiv(perfquery, GL_QUERY_RESULT, &queryresult);
	}

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	glActiveTexture(GL_TEXTURE0);

	// present
	Math::MatrixIdentity(world);
	screenquad->SetTextureMatrix(world);

	glEnable(GL_FRAMEBUFFER_SRGB);

	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, rendertarget);
	screenquad->Draw();

	glDisable(GL_FRAMEBUFFER_SRGB);

	if (false) {
		// debug draw
		debugdraw->SetMatrix("matViewProj", viewproj);

		glEnable(GL_DEPTH_TEST);

		debugdraw->Begin();
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, accelstructure->GetHierarchy());
			debugmesh->DrawInstanced(accelstructure->GetNumNodes());
		}
		debugdraw->End();
	}

	time += elapsedtime;
	
	if (drawtext) {
		if (time - lasttime > 0.5f) {
			UpdateText();
			lasttime = time;
		}
	
		// render text
		glViewport(10, screenheight - 522, 512, 512);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, helptext);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();
	}

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
	//app = Application::Create(768, 432);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGL)) {
		delete app;
		return 1;
	}

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyDownCallback = KeyDown;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;

	app->Run();
	delete app;

	return 0;
}
