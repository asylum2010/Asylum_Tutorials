
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\geometryutils.h"
#include "..\Common\basiccamera.h"

#define GRID_SIZE			20		// static batching won't work for more
#define OBJECT_SCALE		0.75f

// helper macros
#define TITLE				"Shader sample 51: Instancing methods"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			mesh			= nullptr;
OpenGLMesh*			staticbatch		= nullptr;	// for static instancing
OpenGLEffect*		kitcheneffect	= nullptr;
OpenGLEffect*		instanceeffect	= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				hwinstancevao	= 0;
GLuint				instancebuffer	= 0;
GLuint				helptext		= 0;

BasicCamera			camera;
int					rendermethod	= 1;

typedef GeometryUtils::CommonVertex CommonVertex;

void CreateStaticBatch();
void CreateInstancedLayout();

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

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/beanbag2.qm", &mesh)) {
		MYERROR("Could not load mesh");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong.frag", &kitcheneffect)) {
		MYERROR("Could not load 'blinnphong' effect");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong.frag", &instanceeffect, "#define HARDWARE_INSTANCING\n")) {
		MYERROR("Could not load 'instanced blinnphong' effect");
		return false;
	}

	screenquad = new OpenGLScreenQuad();

	// setup static instancing
	std::cout << "Generating static batch...";
	CreateStaticBatch();
	std::cout << "done\n";

	// setup hardware instancing
	CreateInstancedLayout();

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"1 - Kitchen instancing\n2 - Static instancing\n3 - Hardware instancing",
		helptext, 512, 512);

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(80));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(GRID_SIZE);
	camera.SetOrientation(Math::DegreesToRadians(-155), Math::DegreesToRadians(30), 0);

	return true;
}

void CreateStaticBatch()
{
	OpenGLAttributeRange*	subsettable			= mesh->GetAttributeTable();
	OpenGLAttributeRange*	batchsubsettable	= new OpenGLAttributeRange[mesh->GetNumSubsets()];
	CommonVertex*			vdata				= nullptr;
	CommonVertex*			batchvdata			= nullptr;
	GLushort*				idata				= nullptr;
	GLuint*					batchidata			= nullptr;
	GLuint					numsubsets			= mesh->GetNumSubsets();
	GLuint					count				= GRID_SIZE * GRID_SIZE * GRID_SIZE;
	Math::Matrix			world, worldinv;

	OpenGLVertexElement elem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
		{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	GLCreateMesh(mesh->GetNumVertices() * count, mesh->GetNumIndices() * count, GLMESH_32BIT, elem, &staticbatch);

	mesh->LockVertexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&vdata);
	mesh->LockIndexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&idata);

	Math::MatrixScaling(world, OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE);

	staticbatch->LockVertexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchvdata);
	staticbatch->LockIndexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchidata);
	{
		// pre-transform vertices
		for (int i = 0; i < GRID_SIZE; ++i) {
			for (int j = 0; j < GRID_SIZE; ++j) {
				for (int k = 0; k < GRID_SIZE; ++k) {
					world._41 = GRID_SIZE * -0.5f + i;
					world._42 = GRID_SIZE * -0.5f + j;
					world._43 = GRID_SIZE * -0.5f + k;

					Math::MatrixInverse(worldinv, world);

					for (GLuint l = 0; l < mesh->GetNumVertices(); ++l) {
						const CommonVertex& vert = vdata[l];
						CommonVertex& batchvert = batchvdata[l];

						Math::Vec3TransformCoord((Math::Vector3&)batchvert.x, (const Math::Vector3&)vert.x, world);
						Math::Vec3TransformNormalTranspose((Math::Vector3&)batchvert.nx, worldinv, (const Math::Vector3&)vert.nx);
					}

					batchvdata += mesh->GetNumVertices();
				}
			}
		}

		// group indices by subset
		GLuint indexoffset = 0;

		for (GLuint s = 0; s < numsubsets; ++s) {
			const OpenGLAttributeRange& subset = subsettable[s];
			OpenGLAttributeRange& batchsubset = batchsubsettable[s];

			for (int i = 0; i < GRID_SIZE; ++i) {
				for (int j = 0; j < GRID_SIZE; ++j) {
					for (int k = 0; k < GRID_SIZE; ++k) {
						GLuint vertexoffset = (i * GRID_SIZE * GRID_SIZE + j * GRID_SIZE + k) * mesh->GetNumVertices();

						for (GLuint l = 0; l < subset.IndexCount; ++l)
							batchidata[l] = idata[subset.IndexStart + l] + vertexoffset;

						batchidata += subset.IndexCount;
					}
				}
			}

			batchsubset.AttribId		= subset.AttribId;
			batchsubset.Enabled			= subset.Enabled;
			batchsubset.PrimitiveType	= subset.PrimitiveType;

			batchsubset.IndexStart		= indexoffset;
			batchsubset.IndexCount		= subset.IndexCount * count;
			batchsubset.VertexStart		= 0;
			batchsubset.VertexCount		= 0;

			indexoffset += count * subset.IndexCount;
		}
	}
	staticbatch->UnlockIndexBuffer();
	staticbatch->UnlockVertexBuffer();

	staticbatch->SetAttributeTable(batchsubsettable, numsubsets);

	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	delete[] batchsubsettable;
}

void CreateInstancedLayout()
{
	Math::Matrix world;
	GLuint instancebuffersize = GRID_SIZE * GRID_SIZE * GRID_SIZE * 80;

	glGenBuffers(1, &instancebuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instancebuffer);
	glBufferData(GL_ARRAY_BUFFER, instancebuffersize, NULL, GL_STATIC_DRAW);

	Math::MatrixScaling(world, OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE);

	float* mdata = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	
	for (int i = 0; i < GRID_SIZE; ++i) {
		for (int j = 0; j < GRID_SIZE; ++j) {
			for (int k = 0; k < GRID_SIZE; ++k) {
				world._41 = GRID_SIZE * -0.5f + i;
				world._42 = GRID_SIZE * -0.5f + j;
				world._43 = GRID_SIZE * -0.5f + k;

				// transform
				((Math::Matrix*)mdata)->operator =(world);
				mdata += 16;

				// color
				mdata[0] = mdata[1] = mdata[2] = mdata[3] = 1;
				mdata += 4;
			}
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &hwinstancevao);
	glBindVertexArray(hwinstancevao);
	{
		// vertex layout
		glBindBuffer(GL_ARRAY_BUFFER, mesh->GetVertexBuffer());

		glEnableVertexAttribArray(GLDECLUSAGE_POSITION);
		glEnableVertexAttribArray(GLDECLUSAGE_NORMAL);
		glEnableVertexAttribArray(GLDECLUSAGE_TEXCOORD);

		glVertexAttribPointer(GLDECLUSAGE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), (const GLvoid*)0);
		glVertexAttribPointer(GLDECLUSAGE_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), (const GLvoid*)12);
		glVertexAttribPointer(GLDECLUSAGE_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), (const GLvoid*)24);

		// instance layout
		glBindBuffer(GL_ARRAY_BUFFER, instancebuffer);

		glEnableVertexAttribArray(6);
		glEnableVertexAttribArray(7);
		glEnableVertexAttribArray(8);
		glEnableVertexAttribArray(9);
		glEnableVertexAttribArray(10);

		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 80, (const GLvoid*)0);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 80, (const GLvoid*)16);
		glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 80, (const GLvoid*)32);
		glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 80, (const GLvoid*)48);
		glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 80, (const GLvoid*)64);

		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);
		glVertexAttribDivisor(8, 1);
		glVertexAttribDivisor(9, 1);
		glVertexAttribDivisor(10, 1);
	}
	glBindVertexArray(0);
}

void UninitScene()
{
	delete mesh;
	delete staticbatch;

	delete kitcheneffect;
	delete instanceeffect;
	delete screenquad;

	glDeleteVertexArrays(1, &hwinstancevao);

	GL_SAFE_DELETE_BUFFER(instancebuffer);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		rendermethod = 1;
		break;

	case KeyCode2:
		rendermethod = 2;
		break;

	case KeyCode3:
		rendermethod = 3;
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

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix identity;
	Math::Matrix world, view, proj;
	Math::Matrix viewproj;
	Math::Vector3 eye;
	Math::Vector3 lightpos = { 6, 3, -10 };
	Math::Color color = { 1, 1, 1, 1 };

	Math::Vec3Scale(lightpos, lightpos, GRID_SIZE / 5.0f);

	if (rendermethod == 1)
		color = Math::Color(1, 0, 0, 1);
	else if (rendermethod == 2)
		color = Math::Color(1, 1, 0, 1);
	else if (rendermethod == 3)
		color = Math::Color(0, 1, 0, 1);

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixScaling(world, OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE);
	Math::MatrixIdentity(identity);

	OpenGLEffect* effect = ((rendermethod == 3) ? instanceeffect : kitcheneffect);

	effect->SetMatrix("matWorld", identity);
	effect->SetMatrix("matViewProj", viewproj);
	effect->SetVector("lightPos", lightpos);
	effect->SetVector("eyePos", eye);
	effect->SetVector("matColor", &color.r);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	effect->Begin();
	{
		if (rendermethod == 1) {
			// kitchen instancing
			for (int i = 0; i < GRID_SIZE; ++i) {
				for (int j = 0; j < GRID_SIZE; ++j) {
					for (int k = 0; k < GRID_SIZE; ++k) {
						world._41 = GRID_SIZE * -0.5f + i;
						world._42 = GRID_SIZE * -0.5f + j;
						world._43 = GRID_SIZE * -0.5f + k;

						effect->SetMatrix("matWorld", world);
						effect->CommitChanges();

						mesh->Draw();
					}
				}
			}
		} else if (rendermethod == 2) {
			// static instancing
			staticbatch->Draw();
		} else if (rendermethod == 3) {
			// hardware instancing
			glBindVertexArray(hwinstancevao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->GetIndexBuffer());

			const OpenGLAttributeRange* subsettable = mesh->GetAttributeTable();

			for (GLuint i = 0; i < mesh->GetNumSubsets(); ++i) {
				const OpenGLAttributeRange& subset = subsettable[i];
				GLuint start = subset.IndexStart * ((mesh->GetIndexType() == GL_UNSIGNED_INT) ? 4 : 2);

				// this is where I tell how many instances to render
				glDrawElementsInstanced(subset.PrimitiveType, subset.IndexCount, mesh->GetIndexType(), (char*)0 + start, GRID_SIZE * GRID_SIZE * GRID_SIZE);
			}
		}
	}
	effect->End();

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

	app->Run();
	delete app;

	return 0;
}
