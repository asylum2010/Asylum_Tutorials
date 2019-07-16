
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\geometryutils.h"
#include "..\Common\basiccamera.h"

#define GRID_SIZE			10
#define BEANBAG_SCALE		0.75f
#define TEAPOT_SCALE		0.35f
#define ANGEL_SCALE			0.003f

// helper macros
#define TITLE				"Shader sample 51: Multi-draw indirect"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app					= nullptr;

OpenGLMesh*			meshes[3]			= { nullptr };
OpenGLMesh*			staticbatches[3]	= { nullptr };	// for static batching
OpenGLMesh*			multibatch			= nullptr;		// for multi-draw
OpenGLEffect*		kitcheneffect		= nullptr;
OpenGLEffect*		instanceeffect		= nullptr;
OpenGLEffect*		computeprogram		= nullptr;
OpenGLScreenQuad*	screenquad			= nullptr;

GLuint				hwinstancevao		= 0;
GLuint				instancebuffer		= 0;
GLuint				indirectbuffer		= 0;
GLuint				parameterbuffer		= 0;
GLuint				helptext			= 0;

BasicCamera			camera;
float				meshscales[3]		= { BEANBAG_SCALE, TEAPOT_SCALE, ANGEL_SCALE };
Math::Color			meshcolors[3]		= { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
int					rendermethod		= 1;

typedef GeometryUtils::CommonVertex CommonVertex;

void CreateMultiDrawBatch();
void CreateStaticBatches();
void CreateIndirectCommandBuffer();
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

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/beanbag2.qm", &meshes[0])) {
		MYERROR("Could not load mesh");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/teapot.qm", &meshes[1])) {
		MYERROR("Could not load mesh");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/angel.qm", &meshes[2])) {
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
	CreateStaticBatches();
	std::cout << "done\n";

	std::cout << "Generating multi-draw static batch...";
	CreateMultiDrawBatch();
	std::cout << "done\n";

	// setup indirect draw
	CreateIndirectCommandBuffer();

	// setup hardware instancing
	CreateInstancedLayout();

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	if (GLExtensions::ARB_indirect_parameters && GLExtensions::ARB_compute_shader ) {
		if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/indirectcommands.comp", &computeprogram)) {
			MYERROR("Could not load compute shader");
			return false;
		}

		GLRenderText(
			"1 - Kitchen draw\n2 - Static batching\n3 - Multi-draw (selective draw from SB)\n4 - Multi-draw indirect (instanced SBing)\n5 - Indirect parameters",
			helptext, 512, 512);
	} else {
		GLRenderText(
			"1 - Kitchen draw\n2 - Static batching\n3 - Multi-draw (selective draw from SB)\n4 - Multi-draw indirect (instanced SBing)",
			helptext, 512, 512);
	}

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(80));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(GRID_SIZE);
	camera.SetOrientation(Math::DegreesToRadians(-155), Math::DegreesToRadians(30), 0);

	return true;
}

void CreateMultiDrawBatch()
{
	Math::Matrix	world, worldinv;
	CommonVertex*	vdata				= nullptr;
	CommonVertex*	batchvdata			= nullptr;
	GLushort*		idata				= nullptr;
	GLuint*			batchidata			= nullptr;
	GLuint			numvertices			= 0;
	GLuint			numindices			= 0;

	for (int m = 0; m < 3; ++m) {
		numvertices += meshes[m]->GetNumVertices();
		numindices += meshes[m]->GetNumIndices();
	}

	OpenGLVertexElement elem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
		{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	GLCreateMesh(numvertices, numindices, GLMESH_32BIT, elem, &multibatch);

	multibatch->LockVertexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchvdata);
	multibatch->LockIndexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchidata);
	{
		GLuint vertexoffset = 0;

		for (int m = 0; m < 3; ++m) {
			meshes[m]->LockVertexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&vdata);
			meshes[m]->LockIndexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&idata);

			Math::MatrixScaling(world, meshscales[m], meshscales[m], meshscales[m]);
			Math::MatrixInverse(worldinv, world);

			// pre-scale vertices
			for (GLuint l = 0; l < meshes[m]->GetNumVertices(); ++l) {
				const CommonVertex& vert = vdata[l];
				CommonVertex& batchvert = batchvdata[l];

				Math::Vec3TransformCoord((Math::Vector3&)batchvert.x, (const Math::Vector3&)vert.x, world);
				Math::Vec3TransformNormalTranspose((Math::Vector3&)batchvert.nx, worldinv, (const Math::Vector3&)vert.nx);
			}

			batchvdata += meshes[m]->GetNumVertices();

			for (GLuint l = 0; l < meshes[m]->GetNumIndices(); ++l) {
				batchidata[l] = idata[l] + vertexoffset;
			}

			batchidata += meshes[m]->GetNumIndices();
			vertexoffset += meshes[m]->GetNumVertices();

			meshes[m]->UnlockIndexBuffer();
			meshes[m]->UnlockVertexBuffer();
		}
	}
	multibatch->UnlockIndexBuffer();
	multibatch->UnlockVertexBuffer();

	// NOTE: ignore subset table
}

void CreateStaticBatches()
{
	GLuint totalcount = GRID_SIZE * GRID_SIZE * GRID_SIZE;

	for (int m = 0; m < 3; ++m) {
		Math::Matrix			world, worldinv;
		OpenGLAttributeRange*	subsettable			= meshes[m]->GetAttributeTable();
		OpenGLAttributeRange*	batchsubsettable	= new OpenGLAttributeRange[meshes[m]->GetNumSubsets()];
		CommonVertex*			vdata				= nullptr;
		CommonVertex*			batchvdata			= nullptr;
		GLushort*				idata				= nullptr;
		GLuint*					batchidata			= nullptr;
		GLuint					numsubsets			= meshes[m]->GetNumSubsets();
		GLuint					count				= totalcount / 3 + ((m == 0) ? 1 : 0);

		OpenGLVertexElement elem[] = {
			{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
			{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
			{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
			{ 0xff, 0, 0, 0, 0 }
		};

		GLCreateMesh(meshes[m]->GetNumVertices() * count, meshes[m]->GetNumIndices() * count, GLMESH_32BIT, elem, &staticbatches[m]);

		meshes[m]->LockVertexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&vdata);
		meshes[m]->LockIndexBuffer(0, 0, GL_MAP_READ_BIT, (void**)&idata);

		Math::MatrixScaling(world, meshscales[m], meshscales[m], meshscales[m]);

		staticbatches[m]->LockVertexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchvdata);
		staticbatches[m]->LockIndexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&batchidata);
		{
			// pre-transform vertices
			for (int i = 0; i < GRID_SIZE; ++i) {
				for (int j = 0; j < GRID_SIZE; ++j) {
					for (int k = 0; k < GRID_SIZE; ++k) {
						GLuint index = i * GRID_SIZE * GRID_SIZE + j * GRID_SIZE + k;

						if (index % 3 != m)
							continue;

						world._41 = GRID_SIZE * -0.5f + i;
						world._42 = GRID_SIZE * -0.5f + j;
						world._43 = GRID_SIZE * -0.5f + k;

						Math::MatrixInverse(worldinv, world);

						for (GLuint l = 0; l < meshes[m]->GetNumVertices(); ++l) {
							const CommonVertex& vert = vdata[l];
							CommonVertex& batchvert = batchvdata[l];

							Math::Vec3TransformCoord((Math::Vector3&)batchvert.x, (const Math::Vector3&)vert.x, world);
							Math::Vec3TransformNormalTranspose((Math::Vector3&)batchvert.nx, worldinv, (const Math::Vector3&)vert.nx);
						}

						batchvdata += meshes[m]->GetNumVertices();
					}
				}
			}

			// group indices by subset
			GLuint indexoffset = 0;

			for (GLuint s = 0; s < numsubsets; ++s) {
				const OpenGLAttributeRange& subset = subsettable[s];
				OpenGLAttributeRange& batchsubset = batchsubsettable[s];

				for (GLuint i = 0; i < count; ++i) {
					GLuint vertexoffset = i * meshes[m]->GetNumVertices();

					for (GLuint l = 0; l < subset.IndexCount; ++l)
						batchidata[l] = idata[subset.IndexStart + l] + vertexoffset;

					batchidata += subset.IndexCount;
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
		staticbatches[m]->UnlockIndexBuffer();
		staticbatches[m]->UnlockVertexBuffer();

		staticbatches[m]->SetAttributeTable(batchsubsettable, numsubsets);

		meshes[m]->UnlockIndexBuffer();
		meshes[m]->UnlockVertexBuffer();

		delete[] batchsubsettable;
	}
}

void CreateIndirectCommandBuffer()
{
	GLuint totalcount = GRID_SIZE * GRID_SIZE * GRID_SIZE;

	glGenBuffers(1, &indirectbuffer);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectbuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, 3 * sizeof(DrawElementsIndirectCommand), NULL, GL_STATIC_DRAW);

	DrawElementsIndirectCommand* cmddata = (DrawElementsIndirectCommand*)glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY);
	{
		cmddata[0].count			= meshes[0]->GetNumIndices();
		cmddata[0].instanceCount	= (totalcount / 3) + 1;
		cmddata[0].firstIndex		= 0;
		cmddata[0].baseVertex		= 0;
		cmddata[0].baseInstance		= 0;

		cmddata[1].count			= meshes[1]->GetNumIndices();
		cmddata[1].instanceCount	= (totalcount / 3);
		cmddata[1].firstIndex		= meshes[0]->GetNumIndices();
		cmddata[1].baseVertex		= 0;
		cmddata[1].baseInstance		= (totalcount / 3) + 1;

		cmddata[2].count			= meshes[2]->GetNumIndices();
		cmddata[2].instanceCount	= (totalcount / 3);
		cmddata[2].firstIndex		= meshes[0]->GetNumIndices() + meshes[1]->GetNumIndices();
		cmddata[2].baseVertex		= 0;
		cmddata[2].baseInstance		= 2 * (totalcount / 3) + 1;
	}
	glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	// for ARB_indirect_parameters
	glGenBuffers(1, &parameterbuffer);

	glBindBuffer(GL_PARAMETER_BUFFER, parameterbuffer);
	glBufferData(GL_PARAMETER_BUFFER, sizeof(GLsizei), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_PARAMETER_BUFFER, 0);
}

void CreateInstancedLayout()
{
	Math::Matrix world;
	GLuint totalcount = GRID_SIZE * GRID_SIZE * GRID_SIZE;
	GLuint instancebuffersize = totalcount * 80;

	glGenBuffers(1, &instancebuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instancebuffer);
	glBufferData(GL_ARRAY_BUFFER, instancebuffersize, NULL, GL_STATIC_DRAW);

	Math::MatrixIdentity(world);

	float* mdata = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	
	GLuint counts[] = {
		totalcount / 3 + 1,
		totalcount / 3,
		totalcount / 3
	};

	for (GLuint m = 0; m < 3; ++m) {
		for (GLuint l = 0; l < counts[m]; ++l) {
			GLuint index = l * 3 + m;

			GLuint i = index / (GRID_SIZE * GRID_SIZE);
			GLuint j = (index % (GRID_SIZE * GRID_SIZE)) / GRID_SIZE;
			GLuint k = index % GRID_SIZE;

			world._41 = GRID_SIZE * -0.5f + i;
			world._42 = GRID_SIZE * -0.5f + j;
			world._43 = GRID_SIZE * -0.5f + k;

			// transform
			((Math::Matrix*)mdata)->operator =(world);
			mdata += 16;

			// color
			((Math::Color*)mdata)->operator =(meshcolors[(index + 2) % 3]);
			mdata += 4;
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &hwinstancevao);
	glBindVertexArray(hwinstancevao);
	{
		// vertex layout
		glBindBuffer(GL_ARRAY_BUFFER, multibatch->GetVertexBuffer());

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
	delete staticbatches[0];
	delete staticbatches[1];
	delete staticbatches[2];
	delete multibatch;
	delete meshes[0];
	delete meshes[1];
	delete meshes[2];

	delete kitcheneffect;
	delete instanceeffect;
	delete computeprogram;
	delete screenquad;

	glDeleteVertexArrays(1, &hwinstancevao);

	GL_SAFE_DELETE_BUFFER(instancebuffer);
	GL_SAFE_DELETE_BUFFER(indirectbuffer);
	GL_SAFE_DELETE_BUFFER(parameterbuffer);
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
		if (computeprogram != nullptr)
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
	static float time = 0;

	Math::Matrix identity;
	Math::Matrix world, view, proj;
	Math::Matrix viewproj;
	Math::Vector3 eye;
	Math::Vector3 lightpos = { 6, 3, -10 };
	Math::Color white = { 1, 1, 1, 1 };

	Math::Vec3Scale(lightpos, lightpos, GRID_SIZE / 5.0f);

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixIdentity(identity);

	if (rendermethod == 5) {
		// generate indirect commands dynamically
		GLuint params[] = {
			meshes[0]->GetNumIndices(),
			meshes[1]->GetNumIndices(),
			meshes[2]->GetNumIndices(),
			GRID_SIZE * GRID_SIZE * GRID_SIZE
		};

		computeprogram->SetFloat("time", time);
		computeprogram->SetUIntVector("params", params);

		computeprogram->Begin();
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, indirectbuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, parameterbuffer);

			glDispatchCompute(1, 1, 1);
		}
		computeprogram->End();

		glMemoryBarrier(GL_COMMAND_BARRIER_BIT); //|GL_PARAMETER_BARRIER_BIT

		time += elapsedtime;
	}

	// render
	OpenGLEffect* effect = ((rendermethod >= 4) ? instanceeffect : kitcheneffect);

	effect->SetMatrix("matWorld", identity);
	effect->SetMatrix("matViewProj", viewproj);
	effect->SetVector("lightPos", lightpos);
	effect->SetVector("eyePos", eye);
	effect->SetVector("matColor", &white.r);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	effect->Begin();
	{
		if (rendermethod == 1) {
			// kitchen draw
			for (int i = 0; i < GRID_SIZE; ++i) {
				for (int j = 0; j < GRID_SIZE; ++j) {
					for (int k = 0; k < GRID_SIZE; ++k) {
						GLuint index = i * GRID_SIZE * GRID_SIZE + j * GRID_SIZE + k;
						float scale = meshscales[index % 3];

						Math::MatrixScaling(world, scale, scale, scale);

						world._41 = GRID_SIZE * -0.5f + i;
						world._42 = GRID_SIZE * -0.5f + j;
						world._43 = GRID_SIZE * -0.5f + k;

						effect->SetMatrix("matWorld", world);
						effect->SetVector("matColor", &meshcolors[index % 3].r);
						effect->CommitChanges();
						
						meshes[index % 3]->Draw();
					}
				}
			}
		} else if (rendermethod == 2) {
			// static batching
			effect->SetVector("matColor", &meshcolors[1].r);
			effect->CommitChanges();

			staticbatches[0]->Draw();

			effect->SetVector("matColor", &meshcolors[2].r);
			effect->CommitChanges();

			staticbatches[1]->Draw();

			effect->SetVector("matColor", &meshcolors[0].r);
			effect->CommitChanges();

			staticbatches[2]->Draw();
		} else if (rendermethod == 3) {
			// multi-draw
			glBindVertexArray(multibatch->GetVertexLayout());
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, multibatch->GetIndexBuffer());

			// render beanbag and angel
			GLsizei counts[2] = { meshes[0]->GetNumIndices(), meshes[2]->GetNumIndices() };
			const GLvoid* indexoffsets[2] = { (const GLvoid*)0, (const GLvoid*)((meshes[0]->GetNumIndices() + meshes[1]->GetNumIndices()) * sizeof(GLuint)) };

			Math::MatrixIdentity(world);

			for (int i = 0; i < GRID_SIZE; ++i) {
				for (int j = 0; j < GRID_SIZE; ++j) {
					for (int k = 0; k < GRID_SIZE; ++k) {
						GLuint index = i * GRID_SIZE * GRID_SIZE + j * GRID_SIZE + k;

						if (index % 3 != 1)
							continue;

						world._41 = GRID_SIZE * -0.5f + i;
						world._42 = GRID_SIZE * -0.5f + j;
						world._43 = GRID_SIZE * -0.5f + k;

						effect->SetMatrix("matWorld", world);
						effect->SetVector("matColor", &meshcolors[((index * 2) / 3) % 3].r);
						effect->CommitChanges();

						glMultiDrawElements(GL_TRIANGLES, counts, multibatch->GetIndexType(), &indexoffsets[0], 2);
					}
				}
			}
		} else if (rendermethod == 4) {
			// multi-draw indirect
			glBindVertexArray(hwinstancevao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, multibatch->GetIndexBuffer());
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectbuffer);

			glMultiDrawElementsIndirect(GL_TRIANGLES, multibatch->GetIndexType(), 0, 3, sizeof(DrawElementsIndirectCommand));
		} else if (rendermethod == 5) {
			// indirect parameters
			glBindVertexArray(hwinstancevao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, multibatch->GetIndexBuffer());
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectbuffer);
			glBindBuffer(GL_PARAMETER_BUFFER, parameterbuffer);

			glMultiDrawElementsIndirectCount(GL_TRIANGLES, multibatch->GetIndexType(), 0, 0, 3, sizeof(DrawElementsIndirectCommand));
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
