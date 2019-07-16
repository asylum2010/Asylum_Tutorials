
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define GRID_SIZE		10
#define UNIFORM_COPIES	512
#define OBJECT_SCALE	0.35f

// helper macros
#define TITLE				"Shader sample 51: Uniform data streaming methods"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

struct UniformBlock
{
	// byte offset 0
	struct VertexUniformData {
		Math::Matrix matWorld;
		Math::Matrix matViewProj;
		Math::Vector4 lightPos;
		Math::Vector4 eyePos;
	} vsuniforms;	// 160 B

	Math::Vector4 padding1[6];	// 96 B

	// byte offset 256
	struct FragmentUniformData {
		Math::Color matColor;
	} fsuniforms;	// 16 B

	Math::Vector4 padding2[15];	// 240 B
};

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			mesh			= nullptr;
OpenGLEffect*		legacy			= nullptr;	// old method
OpenGLEffect*		uniformblock	= nullptr;	// new method
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				uniformbuffer1	= 0;		// single block
GLuint				uniformbuffer2	= 0;		// ringbuffer with fence
GLuint				uniformbuffer3	= 0;		// ringbuffer with persistent mapping
GLuint				helptext		= 0;

BasicCamera			camera;
UniformBlock		uniformDTO;
UniformBlock*		persistentdata	= nullptr;	// for persistent mapping
int					rendermethod	= 1;
int					currentcopy		= 0;	// current block in the ringbuffer

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

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/teapot.qm", &mesh)) {
		MYERROR("Could not load mesh");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong.frag", &legacy)) {
		MYERROR("Could not load 'legacy' effect");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong_ubo.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong_ubo.frag", &uniformblock)) {
		MYERROR("Could not load 'uniformblock' effect");
		return false;
	}

	uniformblock->SetUniformBlockBinding("VertexUniformData", 0);
	uniformblock->SetUniformBlockBinding("FragmentUniformData", 1);

	memset(&uniformDTO, 0, sizeof(uniformDTO));

	// single uniform buffer
	glGenBuffers(1, &uniformbuffer1);
	glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer1);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlock), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// uniform ringbuffer
	glGenBuffers(1, &uniformbuffer2);
	glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer2);

	glBufferData(GL_UNIFORM_BUFFER, UNIFORM_COPIES * sizeof(UniformBlock), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	if (GLExtensions::GLSLVersion >= GLExtensions::GL_4_4) {
		// uniform storage buffer
		glGenBuffers(1, &uniformbuffer3);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer3);

		glBufferStorage(GL_UNIFORM_BUFFER, UNIFORM_COPIES * sizeof(UniformBlock), NULL, GL_DYNAMIC_STORAGE_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

		persistentdata = (UniformBlock*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, UNIFORM_COPIES * sizeof(UniformBlock), GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		assert(persistentdata != nullptr);

		GLRenderText(
			"1 - glUniformXX\n2 - UBO with glBufferSubData\n3 - UBO with glMapBufferRange\n4 - Ring UBO with glFenceSync\n5 - Ring UBO with persistent mapping",
			helptext, 512, 512);
	} else {
		GLRenderText(
			"1 - glUniformXX\n2 - UBO with glBufferSubData\n3 - UBO with glMapBufferRange\n4 - Ring UBO with glFenceSync",
			helptext, 512, 512);
	}

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(80));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(10.0f);

	return true;
}

void UninitScene()
{
	if (persistentdata != nullptr) {
		glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer3);
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		persistentdata = nullptr;
	}

	delete mesh;
	delete legacy;
	delete uniformblock;
	delete screenquad;

	GL_SAFE_DELETE_BUFFER(uniformbuffer1);
	GL_SAFE_DELETE_BUFFER(uniformbuffer2);
	GL_SAFE_DELETE_BUFFER(uniformbuffer3);
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

	case KeyCode4:
		rendermethod = 4;
		break;

	case KeyCode5:
		if (GLExtensions::GLSLVersion >= GLExtensions::GL_4_4)
			rendermethod = 5;

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
	Math::Matrix world, view, proj;
	Math::Vector3 eye;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	uniformDTO.vsuniforms.lightPos = { 6, 3, 10, 1 };
	uniformDTO.vsuniforms.eyePos = { eye[0], eye[1], eye[2], 1 };

	Math::MatrixMultiply(uniformDTO.vsuniforms.matViewProj, view, proj);
	Math::MatrixScaling(uniformDTO.vsuniforms.matWorld, OBJECT_SCALE, OBJECT_SCALE, OBJECT_SCALE);

	switch (rendermethod) {
	case 1:	uniformDTO.fsuniforms.matColor = { 1, 0, 0, 1 }; break;
	case 2:	uniformDTO.fsuniforms.matColor = { 1, 0.5f, 0, 1 }; break;
	case 3:	uniformDTO.fsuniforms.matColor = { 1, 1, 0, 1 }; break;
	case 4:	uniformDTO.fsuniforms.matColor = { 0, 0.75f, 0, 1 }; break;
	case 5:	uniformDTO.fsuniforms.matColor = { 0, 1, 0, 1 }; break;

	default:
		break;
	}

	// render grid
	OpenGLEffect*	effect = ((rendermethod > 1) ? uniformblock : legacy);
	GLsync			sync = nullptr;

	if (rendermethod == 1) {
		legacy->SetMatrix("matViewProj", uniformDTO.vsuniforms.matViewProj);
		legacy->SetVector("lightPos", uniformDTO.vsuniforms.lightPos);
		legacy->SetVector("eyePos", uniformDTO.vsuniforms.eyePos);
		legacy->SetVector("matColor", uniformDTO.fsuniforms.matColor);
	}

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	effect->Begin();
	{
		for (int i = 0; i < GRID_SIZE; ++i) {
			for (int j = 0; j < GRID_SIZE; ++j) {
				for (int k = 0; k < GRID_SIZE; ++k) {
					if (currentcopy >= UNIFORM_COPIES) {
						// buffer is full
						if (rendermethod == 2 || rendermethod == 4) {
							// synchronize with fence
							sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
						} else if (rendermethod == 5) {
							// orphan
							glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer3);
							glUnmapBuffer(GL_UNIFORM_BUFFER);

							persistentdata = (UniformBlock*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, UNIFORM_COPIES * sizeof(UniformBlock),
								GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

							assert(persistentdata != nullptr);

							glBindBuffer(GL_UNIFORM_BUFFER, 0);
							currentcopy = 0;
						}
					}

					uniformDTO.vsuniforms.matWorld._41 = GRID_SIZE * -0.5f + i;
					uniformDTO.vsuniforms.matWorld._42 = GRID_SIZE * -0.5f + j;
					uniformDTO.vsuniforms.matWorld._43 = GRID_SIZE * -0.5f + k;

					if (rendermethod == 1) {
						// old method
						legacy->SetMatrix("matWorld", uniformDTO.vsuniforms.matWorld);
						legacy->CommitChanges();
					} else {
						if (rendermethod == 2) {
							// must wait on GPU before calling glBufferSubData
							if (sync != nullptr) {
								glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
								glDeleteSync(sync);

								sync = nullptr;
							}

							glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer1);
							glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformBlock), &uniformDTO);

							glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformbuffer1, 0, sizeof(UniformBlock::VertexUniformData));
							glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniformbuffer1, offsetof(UniformBlock, fsuniforms), sizeof(UniformBlock::FragmentUniformData));
						} else if (rendermethod == 3) {
							// glMapBufferRange with orphaning
							glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer1);
							void* data = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(UniformBlock), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT);
							{
								memcpy(data, &uniformDTO, sizeof(UniformBlock));
							}
							glUnmapBuffer(GL_UNIFORM_BUFFER);

							glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformbuffer1, 0, sizeof(UniformBlock::VertexUniformData));
							glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniformbuffer1, offsetof(UniformBlock, fsuniforms), sizeof(UniformBlock::FragmentUniformData));
						} else if (rendermethod == 4) {
							// glMapBufferRange with ringbuffer + fence
							GLintptr baseoffset = currentcopy * sizeof(UniformBlock);
							GLbitfield flags = GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT|GL_MAP_UNSYNCHRONIZED_BIT;

							if (sync != nullptr) {
								GLenum result = 0;
								GLbitfield waitflags = GL_SYNC_FLUSH_COMMANDS_BIT;

								do {
									result = glClientWaitSync(sync, waitflags, 500000);
									waitflags = 0;

									if (result == GL_WAIT_FAILED) {
										std::cout << "glClientWaitSync() failed!\n";
										break;
									}
								} while (result == GL_TIMEOUT_EXPIRED);

								// reset fence
								glDeleteSync(sync);
								sync = nullptr;

								currentcopy = 0;
								baseoffset = 0;
							}

							glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer2);
							
							void* data = glMapBufferRange(GL_UNIFORM_BUFFER, baseoffset, sizeof(UniformBlock), flags);
							assert(data != 0);
							{
								memcpy(data, &uniformDTO, sizeof(UniformBlock));
							}
							glUnmapBuffer(GL_UNIFORM_BUFFER);

							glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformbuffer2, baseoffset, sizeof(UniformBlock::VertexUniformData));
							glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniformbuffer2, baseoffset + offsetof(UniformBlock, fsuniforms), sizeof(UniformBlock::FragmentUniformData));

							++currentcopy;
						} else if (rendermethod == 5) {
							// use persistent mapping
							GLintptr baseoffset = currentcopy * sizeof(UniformBlock);
							memcpy(persistentdata + currentcopy, &uniformDTO, sizeof(UniformBlock));

							glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformbuffer3, baseoffset, sizeof(UniformBlock::VertexUniformData));
							glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniformbuffer3, baseoffset + offsetof(UniformBlock, fsuniforms), sizeof(UniformBlock::FragmentUniformData));

							++currentcopy;
						}
					}

					mesh->DrawSubset(0);
				}
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
