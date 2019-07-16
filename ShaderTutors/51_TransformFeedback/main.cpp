
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define NUM_EMITTERS		2
#define MAX_PARTICLES		8192

#define EMIT_RATE			0.2f
#define LIFE_SPAN			7.0f
#define BUOYANCY			0.002f

#define BUDDHA_SCALE		25.0f
#define BOWL_SCALE			0.005f

#define BOWL1_POSITION		-2.5f, 0.0f, -0.5f
#define BOWL2_POSITION		2.5f, 0.0f, -0.5f

// helper macros
#define TITLE				"Shader sample 51: Transform feedback"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct Particle
{
	Math::Vector4 position;
	Math::Vector4 velocity;
	Math::Color color;
};

// sample variables
Application*			app						= nullptr;

OpenGLMesh*				ground					= nullptr;
OpenGLMesh*				buddha					= nullptr;
OpenGLMesh*				bowl					= nullptr;
OpenGLMesh*				sky						= nullptr;

OpenGLEffect*			lambert					= nullptr;
OpenGLEffect*			lambert_textured		= nullptr;
OpenGLEffect*			skyeffect				= nullptr;

OpenGLProgramPipeline*	smokeemitpipeline		= nullptr;
OpenGLProgramPipeline*	smokeupdatepipeline		= nullptr;
OpenGLProgramPipeline*	billboardpipeline		= nullptr;

OpenGLScreenQuad*		screenquad				= nullptr;

GLuint					emittersbuffer			= 0;
GLuint					particlebuffers[3]		= { 0, 0, 0 };
GLuint					transformfeedbacks[3]	= { 0, 0, 0 };

GLuint					randomtex				= 0;
GLuint					smoketex				= 0;
GLuint					gradienttex				= 0;
GLuint					groundtex				= 0;
GLuint					environment				= 0;
GLuint					helptext				= 0;

GLuint					particlelayout			= 0;
GLuint					countquery				= 0;

BasicCamera				camera;
Particle*				emitters				= nullptr;
int						currentbuffer			= 0;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (GLExtensions::GLVersion < GLExtensions::GL_4_4) {
		std::cout << "This sample requires at least GL 4.4\n";
		return false;
	}

	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// create input layout for particles
	glGenVertexArrays(1, &particlelayout);
	glBindVertexArray(particlelayout);
	{
		glBindVertexBuffers(0, 0, NULL, NULL, NULL);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);
		glVertexAttribBinding(2, 0);

		glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 4, GL_FLOAT, GL_FALSE, 16);
		glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 32);
	}
	glBindVertexArray(0);

	// create buffers & transform feedbacks
	glGenBuffers(1, &emittersbuffer);
	glGenBuffers(3, particlebuffers);
	glGenTransformFeedbacks(3, transformfeedbacks);

	emitters = new Particle[NUM_EMITTERS];
	memset(emitters, 0, NUM_EMITTERS * sizeof(Particle));

	glBindBuffer(GL_ARRAY_BUFFER, emittersbuffer);
	glBufferData(GL_ARRAY_BUFFER, NUM_EMITTERS * sizeof(Particle), NULL, GL_DYNAMIC_DRAW);

	for (int i = 0; i < 3; ++i) {
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformfeedbacks[i]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particlebuffers[i]);
	}

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

	glBindBuffer(GL_ARRAY_BUFFER, particlebuffers[0]);
	glBufferStorage(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Particle), NULL, GL_DYNAMIC_STORAGE_BIT|GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, particlebuffers[1]);
	glBufferStorage(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Particle), NULL, GL_DYNAMIC_STORAGE_BIT|GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, particlebuffers[2]);
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Particle), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// create random texture
	float* data = new float[2048 * 4];

	for (int i = 0; i < 2048 * 4; ++i)
		data[i] = Math::RandomFloat();

	glGenTextures(1, &randomtex);

	glBindTexture(GL_TEXTURE_2D, randomtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2048, 1, 0, GL_RGBA, GL_FLOAT, data);

	delete[] data;

	// create pipelines
	smokeemitpipeline = new OpenGLProgramPipeline();
	smokeupdatepipeline = new OpenGLProgramPipeline();
	billboardpipeline = new OpenGLProgramPipeline();

	smokeemitpipeline->AddShader(GL_VERTEX_SHADER, "../../Media/ShadersGL/particle.vert");
	smokeemitpipeline->AddShader(GL_GEOMETRY_SHADER, "../../Media/ShadersGL/smokeemitter.geom");

	if (!smokeemitpipeline->Assemble())
		return false;

	smokeupdatepipeline->AddShader(GL_VERTEX_SHADER, "../../Media/ShadersGL/smokeupdate.vert");
	smokeupdatepipeline->AddShader(GL_GEOMETRY_SHADER, "../../Media/ShadersGL/smokeupdate.geom");

	if (!smokeupdatepipeline->Assemble())
		return false;

	billboardpipeline->AddShader(GL_VERTEX_SHADER, "../../Media/ShadersGL/particle.vert");
	billboardpipeline->AddShader(GL_GEOMETRY_SHADER, "../../Media/ShadersGL/billboard.geom");
	billboardpipeline->AddShader(GL_FRAGMENT_SHADER, "../../Media/ShadersGL/billboard.frag");

	// NOTE: this doesn't work (even tho it should...)
	//billboardpipeline->UseProgramStages(smokeemitpipeline, GL_VERTEX_SHADER_BIT);

	if (!billboardpipeline->Assemble())
		return false;

	// load smoke texture
	GLCreateVolumeTextureFromFile("../../Media/Textures/smokevol1.dds", true, &smoketex);

	glBindTexture(GL_TEXTURE_3D, smoketex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// load gradient texture
	GLCreateTextureFromFile("../../Media/Textures/colorgradient.dds", true, &gradienttex);

	glBindTexture(GL_TEXTURE_2D, gradienttex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// create query
	glGenQueries(1, &countquery);

	// setup scene
	GLCreateMeshFromQM("../../Media/MeshesQM/cylinder.qm", &ground);
	GLCreateMeshFromQM("../../Media/MeshesQM/happy1.qm", &buddha);
	GLCreateMeshFromQM("../../Media/MeshesQM/bowl.qm", &bowl);
	GLCreateMeshFromQM("../../Media/MeshesQM/sky.qm", &sky);

	GLCreateEffectFromFile("../../Media/ShadersGL/lambert.vert", 0, 0, 0, "../../Media/ShadersGL/lambert.frag", &lambert);
	GLCreateEffectFromFile("../../Media/ShadersGL/lambert.vert", 0, 0, 0, "../../Media/ShadersGL/lambert.frag", &lambert_textured, "#define TEXTURED\n");
	GLCreateEffectFromFile("../../Media/ShadersGL/sky.vert", 0, 0, 0, "../../Media/ShadersGL/sky.frag", &skyeffect);

	GLCreateTextureFromFile("../../Media/Textures/wood2.jpg", true, &groundtex);
	GLCreateCubeTextureFromDDS("../../Media/Textures/evening_cloudy.dds", true, &environment);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(80));
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(4);
	camera.SetOrientation(Math::DegreesToRadians(-175), Math::DegreesToRadians(15), 0);

	const Math::AABox& box = buddha->GetBoundingBox();
	float center = (box.Max[1] - box.Min[1]) * 0.5f * BUDDHA_SCALE;

	camera.SetPosition(0, center - 0.2f, 0);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);
	GLRenderText("Mouse left - Orbit camera\nMouse middle - Pan/zoom camera", helptext, 512, 512);

	return true;
}

void UninitScene()
{
	delete[] emitters;

	delete ground;
	delete buddha;
	delete bowl;
	delete sky;
	delete lambert;
	delete lambert_textured;
	delete skyeffect;

	delete smokeemitpipeline;
	delete smokeupdatepipeline;
	delete billboardpipeline;
	delete screenquad;

	glDeleteVertexArrays(1, &particlelayout);
	glDeleteTransformFeedbacks(3, transformfeedbacks);
	glDeleteQueries(1, &countquery);

	glDeleteBuffers(1, &emittersbuffer);
	glDeleteBuffers(3, particlebuffers);

	GL_SAFE_DELETE_TEXTURE(groundtex);
	GL_SAFE_DELETE_TEXTURE(randomtex);
	GL_SAFE_DELETE_TEXTURE(gradienttex);
	GL_SAFE_DELETE_TEXTURE(smoketex);
	GL_SAFE_DELETE_TEXTURE(environment);
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
	static float time = 0;
	static bool prevbufferusable = false;

	Math::Matrix	world, view, proj;
	Math::Matrix	worldinv, viewproj;
	Math::Matrix	tmp;

	Math::Vector4	lightpos	= { 1, 0.65f, -0.5f, 0 }; //{ 1, 0.3f, -0.5f, 0 };
	Math::Vector3	eye, vdir;
	Math::Vector2	clipradius;

	Math::Color		ambient		= { 0.01f, 0.01f, 0.01f, 1 };
	Math::Color		color		= { 1.0f, 0.782f, 0.344f, 1.0f };

	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();
	float hfov = Math::HorizontalFov(camera.GetFov(), (float)screenwidth, (float)screenheight);

	clipradius[0] = 0.4f / tanf(hfov * 0.5f);
	clipradius[1] = 0.4f / tanf(camera.GetFov() * 0.5f);

	time += elapsedtime;

	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);

	// update emitters
	for (int i = 0; i < NUM_EMITTERS; ++i) {
		emitters[i].velocity[3] = ((emitters[i].velocity[3] > EMIT_RATE) ? 0.0f : (emitters[i].velocity[3] + elapsedtime));

		if (i == 0) {
			emitters[i].position = Math::Vector4(BOWL1_POSITION, 1);
			emitters[i].position[1] += 0.75f;
		} else if (i == 1) {
			emitters[i].position = Math::Vector4(BOWL2_POSITION, 1);
			emitters[i].position[1] += 0.75f;
		} else {
			assert(false);
		}
	}

	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, emittersbuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_EMITTERS * sizeof(Particle), emitters);

	// emit particles
	GLintptr offset = 0;
	GLsizei stride = sizeof(Particle);

	glEnable(GL_RASTERIZER_DISCARD);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformfeedbacks[2]);
	{
		glBindTexture(GL_TEXTURE_2D, randomtex);
		glProgramUniform1f(smokeemitpipeline->GetProgram(), 1, time);
		glProgramUniform1f(smokeemitpipeline->GetProgram(), 2, EMIT_RATE);

		smokeemitpipeline->Bind();

		glBindVertexArray(particlelayout);
		glBindVertexBuffers(0, 1, &emittersbuffer, &offset, &stride);

		glBeginTransformFeedback(GL_POINTS);
		{
			glDrawArrays(GL_POINTS, 0, NUM_EMITTERS);
		}
		glEndTransformFeedback();
	}

	// update particles
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformfeedbacks[currentbuffer]);
	{
		glProgramUniform1f(smokeupdatepipeline->GetProgram(), 0, elapsedtime);
		glProgramUniform1f(smokeupdatepipeline->GetProgram(), 1, BUOYANCY);
		glProgramUniform1f(smokeupdatepipeline->GetProgram(), 2, LIFE_SPAN);

		smokeupdatepipeline->Bind();

		glBindVertexArray(particlelayout);
		glBindVertexBuffers(0, 1, &particlebuffers[2], &offset, &stride);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countquery);
		glBeginTransformFeedback(GL_POINTS);
		{
			// update and add newly generated particles
			glDrawTransformFeedback(GL_POINTS, transformfeedbacks[2]);

			// update and add older particles
			if (prevbufferusable) {
				glBindVertexBuffers(0, 1, &particlebuffers[1 - currentbuffer], &offset, &stride);
				glDrawTransformFeedback(GL_POINTS, transformfeedbacks[1 - currentbuffer]);
			}
		}
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	}
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	glDisable(GL_RASTERIZER_DISCARD);

	// sort particles (TODO: bitonic sort on GPU)
	GLuint count = 0;
	glGetQueryObjectuiv(countquery, GL_QUERY_RESULT, &count);

	camera.GetPosition(vdir);

	Math::Vec3Subtract(vdir, vdir, eye);
	Math::Vec3Normalize(vdir, vdir);

	if (count > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, particlebuffers[currentbuffer]);
		Particle* data = (Particle*)glMapBufferRange(GL_ARRAY_BUFFER, 0, count * sizeof(Particle), GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
		{
			Particle tmp;

			auto Distance = [&](const Particle& p) -> float {
				Math::Vector3 pdir;
				Math::Vec3Subtract(pdir, (const Math::Vector3&)p.position, eye);

				return Math::Vec3Dot(pdir, vdir);
			};

			for (GLuint i = 1; i < count; ++i) {
				GLuint j = i;

				while (j > 0 && Distance(data[j - 1]) < Distance(data[j])) {
					tmp = data[j - 1];
					data[j - 1] = data[j];
					data[j] = tmp;

					--j;
				}
			}
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// render scene
	glEnable(GL_FRAMEBUFFER_SRGB);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	Math::MatrixScaling(world, 20, 20, 20);

	world._41 = eye[0];
	world._42 = eye[1];
	world._43 = eye[2];

	tmp = proj;

	// project to depth ~1.0
	tmp._33 = -1.0f + 1e-4f;
	tmp._43 = 0;

	Math::MatrixMultiply(tmp, view, tmp);

	skyeffect->SetVector("eyePos", eye);
	skyeffect->SetMatrix("matWorld", world);
	skyeffect->SetMatrix("matViewProj", tmp);

	skyeffect->Begin();
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, environment);
		sky->DrawSubset(0);
	}
	skyeffect->End();

	// ground
	Math::MatrixRotationAxis(tmp, Math::DegreesToRadians(40), 0, 1, 0);
	Math::MatrixScaling(world, 10, 0.1f, 10);
	Math::MatrixMultiply(world, world, tmp);

	world._42 = -0.05f;

	Math::MatrixInverse(worldinv, world);
	Math::MatrixScaling(tmp, 3, 3, 1);

	lambert_textured->SetMatrix("matWorld", world);
	lambert_textured->SetMatrix("matWorldInv", worldinv);
	lambert_textured->SetMatrix("matTexture", tmp);
	lambert_textured->SetMatrix("matViewProj", viewproj);
	lambert_textured->SetVector("lightPos", lightpos);
	lambert_textured->SetVector("matAmbient", &ambient.r);
	lambert_textured->SetInt("sampler0", 0);

	lambert_textured->Begin();
	{
		glBindTexture(GL_TEXTURE_2D, groundtex);
		ground->Draw();
	}
	lambert_textured->End();

	// buddha + bowls
	Math::MatrixScaling(world, BUDDHA_SCALE, BUDDHA_SCALE, BUDDHA_SCALE);

	world._42 = buddha->GetBoundingBox().Min[1] * -BUDDHA_SCALE;
	world._43 = 1.5f;

	Math::MatrixInverse(worldinv, world);

	lambert->SetMatrix("matWorld", world);
	lambert->SetMatrix("matWorldInv", worldinv);
	lambert->SetMatrix("matViewProj", viewproj);
	lambert->SetVector("lightPos", lightpos);
	lambert->SetVector("matAmbient", &ambient.r);

	lambert->Begin();
	{
		for (GLuint i = 0; i < buddha->GetNumSubsets(); ++i) {
			lambert->SetVector("matDiffuse", &color.r);
			lambert->CommitChanges();

			buddha->DrawSubset(i);
		}

		Math::MatrixScaling(world, BOWL_SCALE, BOWL_SCALE, BOWL_SCALE);

		((Math::Vector3&)world._41).operator =(Math::Vector3(BOWL1_POSITION));
		Math::MatrixInverse(worldinv, world);

		lambert->SetMatrix("matWorld", world);
		lambert->SetMatrix("matWorldInv", worldinv);

		for (GLuint i = 0; i < bowl->GetNumSubsets(); ++i) {
			lambert->SetVector("matDiffuse", &bowl->GetMaterialTable()[i].Diffuse.r);
			lambert->CommitChanges();

			bowl->DrawSubset(i);
		}

		((Math::Vector3&)world._41).operator =(Math::Vector3(BOWL2_POSITION));
		Math::MatrixInverse(worldinv, world);

		lambert->SetMatrix("matWorld", world);
		lambert->SetMatrix("matWorldInv", worldinv);

		for (GLuint i = 0; i < bowl->GetNumSubsets(); ++i) {
			lambert->SetVector("matDiffuse", &bowl->GetMaterialTable()[i].Diffuse.r);
			lambert->CommitChanges();

			bowl->DrawSubset(i);
		}
	}
	lambert->End();

	glUseProgram(0);

	// render smoke
	glProgramUniformMatrix4fv(billboardpipeline->GetProgram(), 0, 1, GL_FALSE, viewproj);
	glProgramUniformMatrix4fv(billboardpipeline->GetProgram(), 1, 1, GL_FALSE, view);
	glProgramUniform2fv(billboardpipeline->GetProgram(), 2, 1, clipradius);
	glProgramUniform1f(billboardpipeline->GetProgram(), 3, LIFE_SPAN);
	glProgramUniform1i(billboardpipeline->GetProgram(), 4, 0);
	glProgramUniform1i(billboardpipeline->GetProgram(), 5, 1);

	billboardpipeline->Bind();
	glBindVertexArray(particlelayout);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, smoketex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gradienttex);

	glBindVertexBuffers(0, 1, &particlebuffers[currentbuffer], &offset, &stride);
	glDrawTransformFeedback(GL_POINTS, transformfeedbacks[currentbuffer]);
	
	glActiveTexture(GL_TEXTURE0);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_FRAMEBUFFER_SRGB);

	// advance
	currentbuffer = 1 - currentbuffer;
	prevbufferusable = true;

	// render text
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
