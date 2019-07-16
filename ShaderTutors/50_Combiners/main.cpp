
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\geometryutils.h"

#include <gl/GLU.h>

// helper macros
#define TITLE				"Shader sample 50: GL_EXT_texture_env_combine"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

struct CustomVertex
{
	float x, y, z;
	float nx, ny, nz;	// normal
	float u1, v1;		// texcoord 0
	float u2, v2;		// texcoord 1
	float u3, v3, w3;	// tangent space light vector
	float tx, ty, tz;	// tangent
	float bx, by, bz;	// bitangent
};

// sample variables
Application*		app				= nullptr;
CustomVertex*		vertices		= nullptr;
uint32_t*			indices			= nullptr;

GLuint				logo			= 0;
GLuint				wood			= 0;
GLuint				normalmap		= 0;
GLuint				spheremap		= 0;
GLuint				cubemap			= 0;
GLuint				normcubemap		= 0;	// to normalize vectors on FFP

float lightpos[] = { 6, 3, 10, 1 };
float world[16];

void DrawCube1();	// modulate
void DrawCube2();	// alpha blend
void DrawCube3();	// grayscale
void DrawCube4();	// normal mapping
void DrawCube5();	// sphere mapping
void DrawCube6();	// reflection mapping

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.153f, 0.18f, 0.16f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);

	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	float spec[] = { 1, 1, 1, 1 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

	// create textures
	if (!GLCreateTextureFromFile("../../Media/Textures/gl_logo.png", false, &logo))
		return false;

	if (!GLCreateTextureFromFile("../../Media/Textures/wood.jpg", false, &wood))
		return false;

	if (!GLCreateTextureFromFile("../../Media/Textures/normalmap.jpg", false, &normalmap))
		return false;

	if (!GLCreateTextureFromFile("../../Media/Textures/spheremap1.jpg", false, &spheremap, GLTEX_FLIPY))
		return false;

	const char* files[] = {
		"../../Media/Textures/cubemap1_posx.jpg",
		"../../Media/Textures/cubemap1_negx.jpg",
		"../../Media/Textures/cubemap1_posy.jpg",
		"../../Media/Textures/cubemap1_negy.jpg",
		"../../Media/Textures/cubemap1_posz.jpg",
		"../../Media/Textures/cubemap1_negz.jpg"
	};

	if (!GLCreateCubeTextureFromFiles(files, false, &cubemap))
		return false;

	GLCreateNormalizationCubemap(&normcubemap);

	// create geometry
	GeometryUtils::CommonVertex* vdata1 = new GeometryUtils::CommonVertex[24];
	GeometryUtils::TBNVertex* vdata2 = new GeometryUtils::TBNVertex[24];
	
	vertices = new CustomVertex[24];
	indices = new uint32_t[36];

	GeometryUtils::CreateBox(vdata1, indices, 1, 1, 1);
	GeometryUtils::GenerateTangentFrame(vdata2, vdata1, 24, indices, 36);

	for (int i = 0; i < 24; ++i) {
		vertices[i].x = vdata2[i].x;
		vertices[i].y = vdata2[i].y;
		vertices[i].z = vdata2[i].z;

		vertices[i].nx = vdata2[i].nx;
		vertices[i].ny = vdata2[i].ny;
		vertices[i].nz = vdata2[i].nz;

		vertices[i].tx = vdata2[i].tx;
		vertices[i].ty = vdata2[i].ty;
		vertices[i].tz = vdata2[i].tz;

		vertices[i].bx = vdata2[i].bx;
		vertices[i].by = vdata2[i].by;
		vertices[i].bz = vdata2[i].bz;

		vertices[i].u1 = vertices[i].u2 = vdata2[i].u;
		vertices[i].v1 = vertices[i].v2 = vdata2[i].v;

		vertices[i].u3 = 0;
		vertices[i].v3 = 0;
		vertices[i].w3 = 0;
	}

	delete[] vdata2;
	delete[] vdata1;

	return true;
}

void UninitScene()
{
	delete[] vertices;
	delete[] indices;

	GL_SAFE_DELETE_TEXTURE(logo);
	GL_SAFE_DELETE_TEXTURE(wood);
	GL_SAFE_DELETE_TEXTURE(normalmap);
	GL_SAFE_DELETE_TEXTURE(spheremap);
	GL_SAFE_DELETE_TEXTURE(cubemap);
	GL_SAFE_DELETE_TEXTURE(normcubemap);

	OpenGLContentManager().Release();
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// simplest solution...
	glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	glGetFloatv(GL_MODELVIEW_MATRIX, world);
	glLoadIdentity();

	gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	time += elapsedtime;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)(screenwidth / 3) / (float)(screenheight / 2), 0.1, 100.0);

	glEnable(GL_LIGHTING);

	glViewport(0, 0, screenwidth / 3, screenheight / 2);
	DrawCube1();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(screenwidth / 3, 0, screenwidth / 3, screenheight / 2);
	DrawCube2();

	glDisable(GL_BLEND);

	glViewport(0, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube3();

	glDisable(GL_LIGHTING);

	glViewport(screenwidth / 3, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube4();

	glViewport(2 * screenwidth / 3, 0, screenwidth / 3, screenheight / 2);
	DrawCube5();

	glViewport(2 * screenwidth / 3, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube6();

	app->Present();
}

void DrawCube1()
{
	float* vert = &vertices[0].x;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wood);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawCube2()
{
	float* vert = &vertices[0].x;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wood);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawCube3()
{
	float* vert = &vertices[0].x;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo);

	// multiply by 0.25 and add 0.5
	float scale[] = { 0.25f, 0.25f, 0.25f, 1 };
	float offset[] = { 0.5f, 0.5f, 0.5f, 0 };
	float gray[] = { 0.299f, 0.5f, 0.114f, 1 };

	gray[0] = gray[0] + 0.5f;
	gray[1] = gray[1] + 0.5f;
	gray[2] = gray[2] + 0.5f;

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo); // doesn't matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, scale);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo); // doesn't matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, offset);

	// grayscale
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo); // doesn't matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, gray);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE2);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawCube4()
{
	float ldir[3];
	float wpos[3];
	float wtan[3];
	float wbin[3];
	float wnorm[3];

	// transform light vector to tangent space
	for (int i = 0; i < 24; ++i) {
		CustomVertex& v = vertices[i];

		wpos[0] = v.x * world[0] + v.y * world[4] + v.z * world[8] + world[12];
		wpos[1] = v.x * world[1] + v.y * world[5] + v.z * world[9] + world[13];
		wpos[2] = v.x * world[2] + v.y * world[6] + v.z * world[10] + world[14];

		wtan[0] = v.tx * world[0] + v.ty * world[4] + v.tz * world[8];
		wtan[1] = v.tx * world[1] + v.ty * world[5] + v.tz * world[9];
		wtan[2] = v.tx * world[2] + v.ty * world[6] + v.tz * world[10];

		wbin[0] = v.bx * world[0] + v.by * world[4] + v.bz * world[8];
		wbin[1] = v.bx * world[1] + v.by * world[5] + v.bz * world[9];
		wbin[2] = v.bx * world[2] + v.by * world[6] + v.bz * world[10];

		// this is not correct, but ignore it for now
		wnorm[0] = v.nx * world[0] + v.ny * world[4] + v.nz * world[8];
		wnorm[1] = v.nx * world[1] + v.ny * world[5] + v.nz * world[9];
		wnorm[2] = v.nx * world[2] + v.ny * world[6] + v.nz * world[10];

		ldir[0] = lightpos[0] - wpos[0];
		ldir[1] = lightpos[1] - wpos[1];
		ldir[2] = lightpos[2] - wpos[2];

		// adjust to GL coordinate system
 		v.u3 = -Math::Vec3Dot(wtan, ldir);
		v.v3 = -Math::Vec3Dot(wbin, ldir);
		v.w3 = Math::Vec3Dot(wnorm, ldir);
	}

	float* vert = &vertices[0].x;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8); // texcoord0 (normal map)

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(3, GL_FLOAT, sizeof(CustomVertex), vert + 10); // texcoord1 (light in tangent space)

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6); // texcoord2 (basetex)

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, normalmap);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, normcubemap);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wood);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawCube5()
{
	float* vert = &vertices[0].x;

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, spheremap);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wood);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
}

void DrawCube6()
{
	float* vert = &vertices[0].x;

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, wood);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, logo);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGLDeprecated)) {
		delete app;
		return 1;
	}

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->RenderCallback = Render;

	app->Run();
	delete app;

	return 0;
}
