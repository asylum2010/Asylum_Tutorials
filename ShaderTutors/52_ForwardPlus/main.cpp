
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define NUM_LIGHTS			400		// must be square number
#define LIGHT_RADIUS		1.5f	// must be at least 1
#define SHADOWMAP_SIZE		1024
#define DELAY				5		// in update ticks

// helper macros
#define TITLE				"Shader sample 52: Forward+ rendering"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct LightParticle
{
	Math::Color		color;
	Math::Vector4	previous;
	Math::Vector4	current;
	Math::Vector3	velocity;
	float			radius;
};

struct ObjectInstance
{
	Math::Matrix	transform;
	Math::Vector3	position;
	Math::Vector3	scale;
	OpenGLMesh**	mesh;
	float			angle;
};

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			box				= nullptr;
OpenGLMesh*			teapot			= nullptr;
OpenGLEffect*		lightcull		= nullptr;
OpenGLEffect*		lightaccum		= nullptr;
OpenGLEffect*		ambient			= nullptr;
OpenGLEffect*		blinnphong		= nullptr;
OpenGLEffect*		varianceshadow	= nullptr;
OpenGLEffect*		boxblur3x3		= nullptr;
OpenGLFramebuffer*	framebuffer		= nullptr;
OpenGLFramebuffer*	shadowmap		= nullptr;
OpenGLFramebuffer*	blurredshadow	= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				wood			= 0;
GLuint				marble			= 0;
GLuint				sky				= 0;
GLuint				headbuffer		= 0;	// head of linked lists
GLuint				nodebuffer		= 0;	// nodes of linked lists
GLuint				lightbuffer		= 0;	// light particles
GLuint				counterbuffer	= 0;	// atomic counter
GLuint				helptext		= 0;

BasicCamera			camera;
Math::AABox			scenebox;
GLuint				workgroupsx		= 0;
GLuint				workgroupsy		= 0;
int					timeout			= 0;

ObjectInstance instances[] =
{
	//		position						scaling			mesh		angle
	{ {}, { 0, -0.35f, 0 },				{ 15, 0.5f, 15 },	&box,		0 },

	{ {}, { -3 - 0.108f, -0.108f, -3 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 },
	{ {}, { -3 - 0.108f, -0.108f, 0 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 },
	{ {}, { -3 - 0.108f, -0.108f, 3 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 },

	{ {}, { -0.108f, -0.108f, -3 },		{ 1, 1, 1 },		&teapot,	Math::PI / 4 },
	{ {}, { -0.108f, -0.108f, 0 },		{ 1, 1, 1 },		&teapot,	Math::PI / 4 },
	{ {}, { -0.108f, -0.108f, 3 },		{ 1, 1, 1 },		&teapot,	Math::PI / 4 },

	{ {}, { 3 - 0.108f, -0.108f, -3 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 },
	{ {}, { 3 - 0.108f, -0.108f, 0 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 },
	{ {}, { 3 - 0.108f, -0.108f, 3 },	{ 1, 1, 1 },		&teapot,	Math::PI / -4 }
};

void UpdateParticles(float dt, bool generate);
void RenderScene(OpenGLEffect* effect);

bool InitScene()
{
	if (GLExtensions::GLVersion < GLExtensions::GL_4_3) {
		MYERROR("This sample requires at least OpenGL 4.3");
		return false;
	}

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

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/teapot.qm", &teapot)) {
		MYERROR("Could not load teapot");
		return false;
	}

	// load textures
	if (!GLCreateTextureFromFile("../../Media/Textures/wood2.jpg", true, &wood)) {
		MYERROR("Could not load 'wood' texture");
		return false;
	}

	if (!GLCreateTextureFromFile("../../Media/Textures/marble2.png", true, &marble)) {
		MYERROR("Could not load 'marble' texture");
		return false;
	}

	if (!GLCreateTextureFromFile("../../Media/Textures/static_sky.jpg", true, &sky)) {
		MYERROR("Could not load 'sky' texture");
		return false;
	}

	// calculate scene bounding box
	Math::AABox tmpbox;
	Math::Matrix tmp;

	for (int i = 0; i < ARRAY_SIZE(instances); ++i) {
		ObjectInstance& obj = instances[i];

		// scaling * rotation * translation
		Math::MatrixScaling(tmp, obj.scale[0], obj.scale[1], obj.scale[2]);
		Math::MatrixRotationAxis(obj.transform, obj.angle, 0, 1, 0);
		Math::MatrixMultiply(obj.transform, tmp, obj.transform);

		Math::MatrixTranslation(tmp, obj.position[0], obj.position[1], obj.position[2]);
		Math::MatrixMultiply(obj.transform, obj.transform, tmp);

		tmpbox = (*obj.mesh)->GetBoundingBox();
		tmpbox.TransformAxisAligned(obj.transform);

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// create render targets
	framebuffer = new OpenGLFramebuffer(screenwidth, screenheight);
	framebuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F);
	framebuffer->AttachTexture(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	if (!framebuffer->Validate())
		return false;

	shadowmap = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	shadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);
	shadowmap->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);

	if (!shadowmap->Validate())
		return false;

	blurredshadow = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	blurredshadow->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);

	if (!blurredshadow->Validate())
		return false;

	// create buffers
	workgroupsx = (screenwidth + (screenwidth % 16)) / 16;
	workgroupsy = (screenheight + (screenheight % 16)) / 16;

	size_t numtiles = workgroupsx * workgroupsy;
	size_t headsize = 16;	// start, count, pad, pad
	size_t nodesize = 16;	// light index, next, pad, pad

	glGenBuffers(1, &headbuffer);
	glGenBuffers(1, &nodebuffer);
	glGenBuffers(1, &lightbuffer);
	glGenBuffers(1, &counterbuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * headsize, 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * nodesize * 1024, 0, GL_STATIC_DRAW);	// 4 MB

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(LightParticle), 0, GL_DYNAMIC_DRAW);

	UpdateParticles(0, true);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// load effects
	if( !GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/boxblur3x3.frag", &boxblur3x3)) {
		MYERROR("Could not load blur shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/shadowmap_variance.vert", 0, 0, 0, "../../Media/ShadersGL/shadowmap_variance.frag", &varianceshadow)) {
		MYERROR("Could not load shadowmap shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong_variance.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong_variance.frag", &blinnphong)) {
		MYERROR("Could not load shadowed light shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/ambient.vert", 0, 0, 0, "../../Media/ShadersGL/ambient.frag", &ambient)) {
		MYERROR("Could not load ambient shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/lightaccum.vert", 0, 0, 0, "../../Media/ShadersGL/lightaccum.frag", &lightaccum)) {
		MYERROR("Could not load light accumulation shader");
		return false;
	}

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/lightcull.comp", &lightcull)) {
		MYERROR("Could not load light culling shader");
		return false;
	}

	screenquad = new OpenGLScreenQuad();

	lightcull->SetInt("depthSampler", 0);
	lightcull->SetInt("numLights", NUM_LIGHTS);
	lightaccum->SetInt("sampler0", 0);

	float white[] = { 1, 1, 1, 1 };
	
	varianceshadow->SetInt("isPerspective", 0);

	blinnphong->SetVector("matSpecular", white);
	blinnphong->SetInt("sampler0", 0);
	blinnphong->SetInt("sampler1", 1);
	blinnphong->SetInt("isPerspective", 0);

	boxblur3x3->SetInt("sampler0", 0);

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
	delete lightcull;
	delete lightaccum;
	delete ambient;
	delete blinnphong;
	delete varianceshadow;
	delete boxblur3x3;
	delete teapot;
	delete box;
	delete framebuffer;
	delete shadowmap;
	delete blurredshadow;
	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(wood);
	GL_SAFE_DELETE_TEXTURE(marble);
	GL_SAFE_DELETE_TEXTURE(sky);
	GL_SAFE_DELETE_TEXTURE(helptext);

	GL_SAFE_DELETE_BUFFER(headbuffer);
	GL_SAFE_DELETE_BUFFER(nodebuffer);
	GL_SAFE_DELETE_BUFFER(lightbuffer);
	GL_SAFE_DELETE_BUFFER(counterbuffer);

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

void UpdateParticles(float dt, bool generate)
{
	Math::Vector3 center;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
	LightParticle* particles = (LightParticle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	scenebox.GetCenter(center);

	if (generate) {
		int segments = Math::ISqrt(NUM_LIGHTS);
		float theta, phi;

		Math::Color randomcolors[3] = {
			Math::Color(1, 0, 0, 1),
			Math::Color(0, 1, 0, 1),
			Math::Color(0, 0, 1, 1)
		};

		for (int i = 0; i < segments; ++i) {
			for (int j = 0; j < segments; ++j) {
				LightParticle& p = particles[i * segments + j];

				theta = ((float)j / (segments - 1)) * Math::PI;
				phi = ((float)i / (segments - 1)) * Math::TWO_PI;

				p.previous[0] = center[0];
				p.previous[1] = center[1];
				p.previous[2] = center[2];
				p.previous[3] = 1;

				p.velocity[0] = sinf(theta) * cosf(phi) * 2;
				p.velocity[1] = cosf(theta) * 2;
				p.velocity[2] = sinf(theta) * sinf(phi) * 2;

				p.current[0] = p.previous[0];
				p.current[1] = p.previous[1];
				p.current[2] = p.previous[2];
				p.current[3] = 1;

				p.radius = LIGHT_RADIUS;
				p.color = randomcolors[(i + j) % 3];
			}
		}
	} else {
		Math::Matrix A, invA;
		Math::Vector3 vx, vy, vz;
		Math::Vector3 b;
		Math::Vector4 planes[6];

		Math::Vector4* bestplane = nullptr;
		float denom, energy;
		float toi, besttoi;		// time of impact
		float impulse, noise;
		bool pastcollision;		// when particle tunnels through object

		scenebox.GetPlanes(planes);

		for (int i = 0; i < NUM_LIGHTS; ++i) {
			LightParticle& p = particles[i];

			// integrate
			p.previous[0] = p.current[0];
			p.previous[1] = p.current[1];
			p.previous[2] = p.current[2];

			p.current[0] += p.velocity[0] * dt;
			p.current[1] += p.velocity[1] * dt;
			p.current[2] += p.velocity[2] * dt;

			// detect collision
			besttoi = 2;

			b[0] = p.current[0] - p.previous[0];
			b[1] = p.current[1] - p.previous[1];
			b[2] = p.current[2] - p.previous[2];

			for (int j = 0; j < 6; ++j) {
				// use radius == 0.5
				denom = Math::Vec3Dot(b, (const Math::Vector3&)planes[j]);
				//pastcollision = (Math::Vec3Dot((const Math::Vector3&)p.previous, (const Math::Vector3&)planes[j]) + planes[j].w < 0.5f);
				pastcollision = (Math::PlaneDotCoord(planes[j], (const Math::Vector3&)p.previous) < 0.5f);
				
				if (denom < -1e-4f) {
					toi = (0.5f - Math::Vec3Dot((const Math::Vector3&)p.previous, (const Math::Vector3&)planes[j]) - planes[j].w) / denom;

					if (((toi <= 1 && toi >= 0) ||		// normal case
						(toi < 0 && pastcollision)) &&	// allow past collision
						toi < besttoi)
					{
						besttoi = toi;
						bestplane = &planes[j];
					}
				}
			}

			if (besttoi <= 1) {
				// resolve constraint
				p.current[0] = (1 - besttoi) * p.previous[0] + besttoi * p.current[0];
				p.current[1] = (1 - besttoi) * p.previous[1] + besttoi * p.current[1];
				p.current[2] = (1 - besttoi) * p.previous[2] + besttoi * p.current[2];

				impulse = -Math::Vec3Dot((const Math::Vector3&)(*bestplane), p.velocity);

				// perturb normal vector
				noise = ((rand() % 100) / 100.0f) * Math::PI * 0.333333f - Math::PI * 0.166666f; // [-PI / 6, PI / 6]

				b[0] = cosf(noise + Math::PI * 0.5f);
				b[1] = cosf(noise);
				b[2] = 0;

				Math::Vec3Normalize(vy, (const Math::Vector3&)(*bestplane));
				Math::GetOrthogonalVectors(vx, vz, vy);

				A._11 = vx[0];	A._12 = vy[0];	A._13 = vz[0];	A._14 = 0;
				A._21 = vx[1];	A._22 = vy[1];	A._23 = vz[1];	A._24 = 0;
				A._31 = vx[2];	A._32 = vy[2];	A._33 = vz[2];	A._34 = 0;
				A._41 = 0;		A._42 = 0;		A._43 = 0;		A._44 = 1;

				Math::MatrixInverse(invA, A);
				Math::Vec3TransformNormal(vy, b, invA);

				energy = Math::Vec3Length(p.velocity);

				p.velocity[0] += 2 * impulse * vy[0];
				p.velocity[1] += 2 * impulse * vy[1];
				p.velocity[2] += 2 * impulse * vy[2];

				// must conserve energy
				Math::Vec3Normalize(p.velocity, p.velocity);

				p.velocity[0] *= energy;
				p.velocity[1] *= energy;
				p.velocity[2] *= energy;
			}

#ifdef _DEBUG
			// test if a light fell through
			scenebox.GetCenter(center);

			if (Math::Vec3Distance((const Math::Vector3&)p.current, center) > scenebox.Radius())
				__debugbreak();
#endif
		}
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Update(float delta)
{
	camera.Update(delta);

	if (timeout > DELAY)
		UpdateParticles(delta, false);
	else
		++timeout;
}

void RenderScene(OpenGLEffect* effect)
{
	Math::Matrix worldinv;

	for (int i = 0; i < ARRAY_SIZE(instances); ++i) {
		const ObjectInstance& obj = instances[i];

		Math::MatrixInverse(worldinv, obj.transform);

		effect->SetMatrix("matWorld", obj.transform);
		effect->SetMatrix("matWorldInv", worldinv);

		if (obj.mesh == &box) {
			float uv[] = { 2, 2, 0, 1 };

			effect->SetVector("uv", uv);
			glBindTexture(GL_TEXTURE_2D, wood);
		} else if (obj.mesh == &teapot) {
			float uv[] = { 1, 1, 0, 1 };

			effect->SetVector("uv", uv);
			glBindTexture(GL_TEXTURE_2D, marble);
		}

		effect->CommitChanges();
		(*obj.mesh)->Draw();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix world, view, proj;
	Math::Matrix viewinv, viewproj;
	Math::Matrix lightview, lightproj;
	Math::Matrix lightviewproj;
	Math::Matrix tmp, texmat;

	Math::Vector4 moondir = { -0.25f, 0.65f, -1, 0 };
	Math::Vector3 eye, look;
	Math::Vector2 clipplanes;
	Math::Vector2 lightclip;
	Math::Vector2 screensize = { (float)app->GetClientWidth(), (float)app->GetClientHeight() };

	Math::Color mooncolor = { 0.6f, 0.6f, 1, 1 };
	Math::Color globalambient = { 0.01f, 0.01f, 0.01f, 1.0f };

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetPosition(look);
	camera.GetEyePosition(eye);

	Math::FitToBoxPerspective(clipplanes, eye, look - eye, scenebox);

	camera.SetClipPlanes(clipplanes.x, clipplanes.y);
	camera.GetProjectionMatrix(proj);

	Math::MatrixMultiply(viewproj, view, proj);

	// calculate moonlight direction (let y stay in world space, so we can see shadow)
	Math::MatrixInverse(viewinv, view);
	Math::Vec3TransformNormal((Math::Vector3&)moondir, (const Math::Vector3&)moondir, viewinv);
	Math::Vec3Normalize((Math::Vector3&)moondir, (const Math::Vector3&)moondir);

	moondir.y = 0.65f;
	
	Math::MatrixViewVector(lightview, (const Math::Vector3&)moondir);
	Math::FitToBoxOrtho(lightproj, lightclip, lightview, scenebox);
	Math::MatrixMultiply(lightviewproj, lightview, lightproj);

	// render shadow map
	glClearColor(1, 1, 1, 1);

	varianceshadow->SetMatrix("matViewProj", lightviewproj);
	varianceshadow->SetVector("clipPlanes", lightclip);

	shadowmap->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		varianceshadow->Begin();
		{
			RenderScene(varianceshadow);
		}
		varianceshadow->End();
	}
	shadowmap->Unset();

	glClearColor(0, 0, 0, 1);

	// blur shadow map
	Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
	float texelsize[] = { 1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE };

	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));

	Math::MatrixIdentity(world);

	boxblur3x3->SetMatrix("matTexture", world);
	boxblur3x3->SetVector("texelSize", texelsize);

	blurredshadow->Set();
	{
		boxblur3x3->Begin();
		{
			screenquad->Draw(false);
		}
		boxblur3x3->End();
	}
	blurredshadow->Unset();

	glDepthMask(GL_TRUE);

	// STEP 1: z pass
	ambient->SetMatrix("matViewProj", viewproj);
	ambient->SetVector("matAmbient", globalambient);

	framebuffer->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// draw background first
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, sky);

		float scaledx = 1360.0f * (screensize.y / 768.0f);
		float scale = screensize.x / scaledx;

		Math::MatrixTranslation(tmp, -0.5f, 0, 0);
		Math::MatrixScaling(texmat, scale, 1, 1);
		Math::MatrixMultiply(texmat, tmp, texmat);

		Math::MatrixTranslation(tmp, 0.5f, 0, 0);
		Math::MatrixMultiply(texmat, texmat, tmp);

		Math::MatrixRotationAxis(tmp, Math::PI, 0, 0, 1);
		Math::MatrixMultiply(texmat, texmat, tmp);

		Math::MatrixReflect(world, xzplane);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();

		glEnable(GL_DEPTH_TEST);

		// then fill z-buffer
		ambient->Begin();
		{
			RenderScene(ambient);
		}
		ambient->End();
	}
	framebuffer->Unset();

	// STEP 2: cull lights
	if (timeout > DELAY) {
		lightcull->SetFloat("alpha", alpha);
		lightcull->SetVector("clipPlanes", clipplanes);
		lightcull->SetVector("screenSize", screensize);
		lightcull->SetMatrix("matProj", proj);
		lightcull->SetMatrix("matView", view);
		lightcull->SetMatrix("matViewProj", viewproj);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
		GLuint* counter = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	
		*counter = 0;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, headbuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodebuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightbuffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterbuffer);
		glBindTexture(GL_TEXTURE_2D, framebuffer->GetDepthAttachment());

		lightcull->Begin();
		{
			glDispatchCompute(workgroupsx, workgroupsy, 1);
		}
		lightcull->End();

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	framebuffer->Set();
	{
		// STEP 3: add moonlight with shadow
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		blinnphong->SetMatrix("matViewProj", viewproj);
		blinnphong->SetMatrix("lightViewProj", lightviewproj);
		blinnphong->SetVector("eyePos", eye);
		blinnphong->SetVector("lightPos", moondir);
		blinnphong->SetVector("lightColor", mooncolor);
		blinnphong->SetVector("clipPlanes", lightclip);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blurredshadow->GetColorAttachment(0));
		glActiveTexture(GL_TEXTURE0);

		blinnphong->Begin();
		{
			RenderScene(blinnphong);
		}
		blinnphong->End();

		// STEP 4: accumulate lighting
		if (timeout > DELAY) {
			lightaccum->SetMatrix("matViewProj", viewproj);
			lightaccum->SetVector("eyePos", eye);
			lightaccum->SetFloat("alpha", alpha);
			lightaccum->SetInt("numTilesX", workgroupsx);

			lightaccum->Begin();
			{
				RenderScene(lightaccum);
			}
			lightaccum->End();
		
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
		}

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}
	framebuffer->Unset();

	// put to screen
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, framebuffer->GetColorAttachment(0));

	Math::MatrixIdentity(world);

	screenquad->SetTextureMatrix(world);
	screenquad->Draw();

	glDisable(GL_FRAMEBUFFER_SRGB);

	// render text
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glViewport(10, screenheight - 522, 512, 512);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
