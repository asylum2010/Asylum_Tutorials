
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 72: Thick lines with compute shader"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			mesh			= nullptr;
OpenGLEffect*		thickline		= nullptr;
OpenGLEffect*		simplecolor		= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				vertexbuffer	= 0;		// contains line data
GLuint				helptext		= 0;

BasicCamera			camera;
Math::Vector4		thickness;
uint32_t			numvertices		= 0;

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

	// create geometry
	OpenGLVertexElement inputlayout[] = {
		{ 0, 0, GLDECLTYPE_FLOAT4, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	const uint32_t segments = 16;

	numvertices = segments * 2;
	GLCreateMesh(numvertices * 2, segments * 6, GLMESH_32BIT, inputlayout, &mesh);

	Math::Vector4* vdata = nullptr;
	Math::Vector4 *vert1, *vert2;
	uint32_t* idata = nullptr;

	// create buffer for thick lines
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, numvertices * sizeof(Math::Vector4), NULL, GL_STATIC_DRAW);

	vdata = (Math::Vector4*)glMapBufferRange(GL_ARRAY_BUFFER, 0, numvertices * sizeof(Math::Vector4), GL_MAP_WRITE_BIT);

	mesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		for (uint32_t i = 0; i < segments; ++i) {
			vert1 = (vdata + i * 2 + 0);
			vert2 = (vdata + i * 2 + 1);

			if (i % 2 == 0) {
				vert1->x = sinf((Math::TWO_PI / segments) * i) * 5;
				vert1->y = 0;
				vert1->z = cosf((Math::TWO_PI / segments) * i) * 5;
				vert1->w = 1;

				vert2->x = sinf((Math::TWO_PI / segments) * (i + 1)) * 2;
				vert2->y = 0;
				vert2->z = cosf((Math::TWO_PI / segments) * (i + 1)) * 2;
				vert2->w = 1;
			} else {
				vert1->x = sinf((Math::TWO_PI / segments) * i) * 2;
				vert1->y = 0;
				vert1->z = cosf((Math::TWO_PI / segments) * i) * 2;
				vert1->w = 1;

				vert2->x = sinf((Math::TWO_PI / segments) * (i + 1)) * 5;
				vert2->y = 0;
				vert2->z = cosf((Math::TWO_PI / segments) * (i + 1)) * 5;
				vert2->w = 1;
			}

			idata[i * 6 + 0] = i * 4 + 0;
			idata[i * 6 + 1] = i * 4 + 1;
			idata[i * 6 + 2] = i * 4 + 2;

			idata[i * 6 + 3] = i * 4 + 3;
			idata[i * 6 + 4] = i * 4 + 2;
			idata[i * 6 + 5] = i * 4 + 1;
		}
	}
	mesh->UnlockIndexBuffer();

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/simplecolor.vert", 0, 0, 0, "../../Media/ShadersGL/simplecolor.frag", &simplecolor)) {
		MYERROR("Could not load 'simplecolor' shader");
		return false;
	}

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/thickline.comp", &thickline)) {
		MYERROR("Could not load 'thickline' shader");
		return false;
	}

	Math::Matrix identity(1, 1, 1, 1);
	Math::Vector4 color(0, 1, 0, 1);

	thickness = Math::Vector4(3.0f / screenwidth, 3.0f / screenheight, 9, 0);

	simplecolor->SetMatrix("matWorld", &identity._11);
	simplecolor->SetVector("matColor", &color.x);

	thickline->SetInt("numVertices", (int)numvertices);
	thickline->SetVector("lineThickness", &thickness.x);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::PI / 3.0f);
	camera.SetPosition(0, 0, 0);
	camera.SetOrientation(Math::DegreesToRadians(135), Math::DegreesToRadians(30), 0);
	camera.SetDistance(8);
	camera.SetClipPlanes(0.1f, 20);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Use the mouse to for camera control",
		helptext, 512, 512);

	return true;
}

void UninitScene()
{
	delete mesh;
	delete thickline;
	delete simplecolor;
	delete screenquad;

	GL_SAFE_DELETE_BUFFER(vertexbuffer);
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
	Math::Matrix viewproj, viewprojinv;
	Math::Vector4 nearplane;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixInverse(viewprojinv, viewproj);

	nearplane.x = proj._13;
	nearplane.y = proj._23;
	nearplane.z = proj._33 + proj._34;
	nearplane.w = proj._43;

	Math::PlaneNormalize(nearplane, nearplane);

	// generate thick lines
	thickline->SetMatrix("matView", &view._11);
	thickline->SetMatrix("matProj", &proj._11);
	thickline->SetMatrix("matViewProjInv", &viewprojinv._11);
	thickline->SetVector("nearPlane", nearplane);

	thickline->Begin();
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexbuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mesh->GetVertexBuffer());

		glDispatchCompute((numvertices + 63) / 64, 1, 1);
	}
	thickline->End();

	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	// render mesh
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	simplecolor->SetMatrix("matViewProj", &viewproj._11);

	simplecolor->Begin();
	{
		mesh->Draw();
	}
	simplecolor->End();

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
