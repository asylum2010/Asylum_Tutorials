
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <sstream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define MAX_NUM_SEGMENTS	100

// helper macros
#define TITLE				"Shader sample 52: NURBS tessellation"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// control constants
const GLuint NumControlVertices				= 7;
const GLuint NumControlIndices				= (NumControlVertices - 1) * 2;
const GLuint MaxSplineVertices				= MAX_NUM_SEGMENTS + 1;
const GLuint MaxSplineIndices				= (MaxSplineVertices - 1) * 2;
const GLuint MaxSurfaceVertices				= MaxSplineVertices * MaxSplineVertices;
const GLuint MaxSurfaceIndices				= (MaxSplineVertices - 1) * (MaxSplineVertices - 1) * 6;

// sample structures
struct CurveData
{
	int degree;	// max 3
	Math::Vector4 controlpoints[NumControlVertices];
	float weights[NumControlVertices + 4];
	float knots[11];
};

CurveData curves[] =
{
	// degree 3, "good"
	{
		3,
		{ { 1, 1, 0, 1 }, { 1, 5, 0, 1 }, { 3, 6, 0, 1 }, { 6, 3, 0, 1 }, { 9, 4, 0, 1 }, { 9, 9, 0, 1 }, { 5, 6, 0, 1 } },
		{ 1, 1, 1, 1, 1, 1, 1 },
		{ 0, 0, 0, 0, 0.4f, 0.4f, 0.4f, 1, 1, 1, 1 }
	},

	// degree 3, "bad"
	{
		3,
		{ { 1, 1, 0, 1 }, { 1, 5, 0, 1 }, { 3, 6, 0, 1 }, { 6, 3, 0, 1 }, { 9, 4, 0, 1 }, { 9, 9, 0, 1 }, { 5, 6, 0, 1 } },
		{ 1, 1, 1, 1, 1, 1, 1 },
		{ 0, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1 }
	},

	// degree 2, "good"
	{
		2,
		{ { 1, 1, 0, 1 }, { 1, 5, 0, 1 }, { 3, 6, 0, 1 }, { 6, 3, 0, 1 }, { 9, 4, 0, 1 }, { 9, 9, 0, 1 }, { 5, 6, 0, 1 } },
		{ 1, 1, 1, 1, 1, 1, 1 },
		{ 0, 0, 0, 0.2f, 0.4f, 0.6f, 0.8f, 1, 1, 1 }
	},

	// degree 1
	{
		1,
		{ { 1, 1, 0, 1 }, { 1, 5, 0, 1 }, { 3, 6, 0, 1 }, { 6, 3, 0, 1 }, { 9, 4, 0, 1 }, { 9, 9, 0, 1 }, { 5, 6, 0, 1 } },
		{ 1, 1, 1, 1, 1, 1, 1 },
		{ 0, 0, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 1, 1 }
	},

	// circle
	{
		2,
		{ { 5, 1, 0, 1 }, { 1, 1, 0, 1 }, { 3, 4.46f, 0, 1 }, { 5, 7.92f, 0, 1 }, { 7, 4.46f, 0, 1 }, { 9, 1, 0, 1 }, { 5, 1, 0, 1 }, },
		{ 1, 0.5f, 1, 0.5f, 1, 0.5f, 1 },
		{ 0, 0, 0, 0.33f, 0.33f, 0.67f, 0.67f, 1, 1, 1 }
	},
};

// sample variables
Application*		app					= nullptr;

OpenGLMesh*			supportlines		= nullptr;
OpenGLMesh*			curve				= nullptr;
OpenGLMesh*			surface				= nullptr;
OpenGLEffect*		tessellatecurve		= nullptr;
OpenGLEffect*		tessellatesurface	= nullptr;
OpenGLEffect*		renderpoints		= nullptr;
OpenGLEffect*		renderlines			= nullptr;
OpenGLEffect*		rendersurface		= nullptr;
OpenGLScreenQuad*	screenquad			= nullptr;

GLuint				helptext			= 0;

BasicCamera			camera;
float				selectiondx			= 0;
float				selectiondy			= 0;
int					numsegments			= MAX_NUM_SEGMENTS / 2;
int					currentcurve		= 0;
int					selectedpoint		= -1;
bool				wireframe			= false;
bool				fullscreen			= false;

long				splinevpsize		= 0;
long				surfvpwidth			= 0;
long				surfvpheight		= 0;

bool UpdateControlPoints(float mx, float my);
void ChangeCurve(GLuint newcurve);
void Tessellate();

static void ConvertToSplineViewport(float& x, float& y)
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	// transform to [0, 10] x [0, 10]
	float unitscale = 10.0f / screenwidth;

	// scale it down and offset with 10 pixels
	float vpscale = (float)splinevpsize / (float)screenwidth;
	float vpoffx = 10.0f;
	float vpoffy = (float)screenheight - (float)splinevpsize - 10.0f;

	x = (x - vpoffx) * unitscale * (1.0f / vpscale);
	y = (y - vpoffy) * unitscale * (1.0f / vpscale);
}

static void UpdateText()
{
	std::stringstream ss;
	const CurveData& current = curves[currentcurve];

	ss.precision(1);
	ss << "Knot vector is:  { " << current.knots[0];

	for (GLuint i = 1; i < NumControlVertices + current.degree + 1; ++i)
		ss << ", " << current.knots[i];

	ss << " }\nWeights are:     { " << current.weights[0];

	for (GLuint i = 1; i < NumControlVertices; ++i)
		ss << ", " << current.weights[i];

	ss << " }\n\n1 - " << ARRAY_SIZE(curves);
	ss << " - presets  W - wireframe  F - full window  +/- tessellation level";
	
	GLRenderTextEx(ss.str(), helptext, 800, 130, L"Calibri", false, 1 /*Gdiplus::FontStyleBold*/, 25);
}

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

	// calculate viewport sizes
	long maxw = screenwidth - 350; // assume 320x240 first
	long maxh = screenheight - 130 - 10; // text is fix
	
	splinevpsize = Math::Min<long>(maxw, maxh);
	splinevpsize -= (splinevpsize % 10);

	surfvpwidth = screenwidth - splinevpsize - 30;
	surfvpheight = (surfvpwidth * 3) / 4;

	// create grid & control poly
	Math::Vector4*	vdata	= nullptr;
	GLushort*		idata16	= nullptr;
	GLuint*			idata32	= nullptr;

	OpenGLVertexElement decl1[] = {
		{ 0, 0, GLDECLTYPE_FLOAT4, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	OpenGLVertexElement decl2[] = {
		{ 0, 0, GLDECLTYPE_FLOAT4, GLDECLUSAGE_POSITION, 0 },
		{ 0, 16, GLDECLTYPE_FLOAT4, GLDECLUSAGE_NORMAL, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	if (!GLCreateMesh(44 + NumControlVertices, NumControlIndices, GLMESH_DYNAMIC, decl1, &supportlines)) {
		MYERROR("Could not create support lines");
		return false;
	}

	supportlines->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	supportlines->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata16);
	{
		// grid points
		for (GLuint i = 0; i < 22; i += 2) {
			vdata[i][0] = vdata[i + 1][0] = (float)(i / 2);
			vdata[i][2] = vdata[i + 1][2] = 0;
			vdata[i][3] = vdata[i + 1][3] = 1;

			vdata[i][1] = 0;
			vdata[i + 1][1] = 10;

			vdata[i + 22][1] = vdata[i + 23][1] = (float)(i / 2);
			vdata[i + 22][2] = vdata[i + 23][2] = 0;
			vdata[i + 22][3] = vdata[i + 23][3] = 1;

			vdata[i + 22][0] = 0;
			vdata[i + 23][0] = 10;
		}

		// curve indices
		for (GLuint i = 0; i < NumControlIndices; i += 2) {
			idata16[i] = 44 + i / 2;
			idata16[i + 1] = 44 + i / 2 + 1;
		}
	}
	supportlines->UnlockIndexBuffer();
	supportlines->UnlockVertexBuffer();

	OpenGLAttributeRange table[] = {
		{ GLPT_LINELIST, 0, 0, 0, 0, 44, true },
		{ GLPT_LINELIST, 1, 0, NumControlIndices, 44, NumControlVertices, true }
	};

	supportlines->SetAttributeTable(table, 2);

	// create spline mesh
	if (!GLCreateMesh(MaxSplineVertices, MaxSplineIndices, GLMESH_32BIT, decl1, &curve)) {
		MYERROR("Could not create curve");
		return false;
	}

	OpenGLAttributeRange* subset0 = curve->GetAttributeTable();

	subset0->PrimitiveType = GLPT_LINELIST;
	subset0->IndexCount = 0;

	// create surface
	if (!GLCreateMesh(MaxSurfaceVertices, MaxSurfaceIndices, GLMESH_32BIT, decl2, &surface)) {
		MYERROR("Could not create surface");
		return false;
	}

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/simplecolor.vert", 0, 0, "../../Media/ShadersGL/drawpoints.geom", "../../Media/ShadersGL/simplecolor.frag", &renderpoints)) {
		MYERROR("Could not load point renderer shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/simplecolor.vert", 0, 0, "../../Media/ShadersGL/drawlines.geom", "../../Media/ShadersGL/simplecolor.frag", &renderlines)) {
		MYERROR("Could not load line renderer shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/rendersurface.vert", 0, 0, 0, "../../Media/ShadersGL/rendersurface.frag", &rendersurface)) {
		MYERROR("Could not load surface renderer shader");
		return false;
	}

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/tessellatecurve.comp", &tessellatecurve)) {
		MYERROR("Could not load compute shader");
		return false;
	}

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/tessellatesurface.comp", &tessellatesurface)) {
		MYERROR("Could not load compute shader");
		return false;
	}

	screenquad = new OpenGLScreenQuad();

	// tessellate for the first time
	ChangeCurve(0);
	Tessellate();

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetDistance(10);
	camera.SetOrientation(Math::HALF_PI, 0.5f, 0);
	camera.SetPosition(5, 4, 5);

	// render text
	GLCreateTexture(800, 130, 1, GLFMT_A8B8G8R8, &helptext);
	UpdateText();

	return true;
}

void UninitScene()
{
	delete tessellatesurface;
	delete tessellatecurve;
	delete renderpoints;
	delete renderlines;
	delete rendersurface;
	delete supportlines;
	delete surface;
	delete curve;
	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

bool UpdateControlPoints(float mx, float my)
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	CurveData& current = curves[currentcurve];

	float	sspx = mx;
	float	sspy = screenheight - my - 1;
	float	dist;
	float	radius = 15.0f / (screenheight / 10);
	bool	isselected = false;

	ConvertToSplineViewport(sspx, sspy);

	if (selectedpoint == -1) {
		for (int i = 0; i < NumControlVertices; ++i) {
			selectiondx = current.controlpoints[i][0] - sspx;
			selectiondy = current.controlpoints[i][1] - sspy;
			dist = selectiondx * selectiondx + selectiondy * selectiondy;

			if (dist < radius * radius) {
				selectedpoint = i;
				break;
			}
		}
	}

	isselected = (selectedpoint > -1 && selectedpoint < NumControlVertices);

	if (isselected) {
		current.controlpoints[selectedpoint][0] = Math::Min<float>(Math::Max<float>(selectiondx + sspx, 0), 10);
		current.controlpoints[selectedpoint][1] = Math::Min<float>(Math::Max<float>(selectiondy + sspy, 0), 10);

		float* data = nullptr;

		supportlines->LockVertexBuffer(
			(44 + selectedpoint) * (GLuint)supportlines->GetNumBytesPerVertex(),
			(GLuint)supportlines->GetNumBytesPerVertex(), GLLOCK_DISCARD, (void**)&data);
		{
			data[0] = current.controlpoints[selectedpoint][0];
			data[1] = current.controlpoints[selectedpoint][1];
			data[2] = current.controlpoints[selectedpoint][2];
		}
		supportlines->UnlockVertexBuffer();
	}

	return isselected;
}

void ChangeCurve(GLuint newcurve)
{
	CurveData&	next = curves[newcurve];
	float		(*vdata)[4] = 0;

	supportlines->LockVertexBuffer(44 * 16, 0, GLLOCK_DISCARD, (void**)&vdata);
	{
		for (GLuint i = 0; i < NumControlVertices; ++i) {
			vdata[i][0] = next.controlpoints[i][0];
			vdata[i][1] = next.controlpoints[i][1];
			vdata[i][2] = next.controlpoints[i][2];
			vdata[i][3] = 1;
		}
	}
	supportlines->UnlockVertexBuffer();
	currentcurve = newcurve;

	UpdateText();
}

void Tessellate()
{
	CurveData& current = curves[currentcurve];

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, curve->GetVertexBuffer());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, curve->GetIndexBuffer());

	if( current.degree > 1 )
		tessellatecurve->SetInt("numCurveVertices", numsegments + 1);
	else
		tessellatecurve->SetInt("numCurveVertices", NumControlVertices);

	tessellatecurve->SetInt("numControlPoints", NumControlVertices);
	tessellatecurve->SetInt("degree", current.degree);
	tessellatecurve->SetFloatArray("knots", current.knots, NumControlVertices + current.degree + 1);
	tessellatecurve->SetFloatArray("weights", current.weights, NumControlVertices);
	tessellatecurve->SetVectorArray("controlPoints", &current.controlpoints[0][0], NumControlVertices);

	tessellatecurve->Begin();
	{
		glDispatchCompute(1, 1, 1);
	}
	tessellatecurve->End();

	// update surface cvs
	Math::Vector4* surfacecvs = new Math::Vector4[NumControlVertices * NumControlVertices];
	GLuint index;

	for (GLuint i = 0; i < NumControlVertices; ++i) {
		for (GLuint j = 0; j < NumControlVertices; ++j) {
			index = i * NumControlVertices + j;

			surfacecvs[index][0] = current.controlpoints[i][0];
			surfacecvs[index][2] = current.controlpoints[j][0];
			surfacecvs[index][1] = (current.controlpoints[i][1] + current.controlpoints[j][1]) * 0.5f;
		}
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, surface->GetVertexBuffer());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, surface->GetIndexBuffer());

	if (current.degree > 1) {
		tessellatesurface->SetInt("numVerticesU", numsegments + 1);
		tessellatesurface->SetInt("numVerticesV", numsegments + 1);
	} else {
		tessellatesurface->SetInt("numVerticesU", NumControlVertices);
		tessellatesurface->SetInt("numVerticesV", NumControlVertices);
	}

	tessellatesurface->SetInt("numControlPointsU", NumControlVertices);
	tessellatesurface->SetInt("numControlPointsV", NumControlVertices);
	tessellatesurface->SetInt("degreeU", current.degree);
	tessellatesurface->SetInt("degreeV", current.degree);
	tessellatesurface->SetFloatArray("knotsU", current.knots, NumControlVertices + current.degree + 1);
	tessellatesurface->SetFloatArray("knotsV", current.knots, NumControlVertices + current.degree + 1);
	tessellatesurface->SetFloatArray("weightsU", current.weights, NumControlVertices);
	tessellatesurface->SetFloatArray("weightsV", current.weights, NumControlVertices);
	tessellatesurface->SetVectorArray("controlPoints", &surfacecvs[0][0], NumControlVertices * NumControlVertices);

	tessellatesurface->Begin();
	{
		glDispatchCompute(1, 1, 1);
	}
	tessellatesurface->End();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_ELEMENT_ARRAY_BARRIER_BIT);

	if (current.degree > 1) {
		curve->GetAttributeTable()->IndexCount = numsegments * 2;
		surface->GetAttributeTable()->IndexCount = numsegments * numsegments * 6;
	} else {
		curve->GetAttributeTable()->IndexCount = (NumControlVertices - 1) * 2;
		surface->GetAttributeTable()->IndexCount = (NumControlVertices - 1) * (NumControlVertices - 1) * 6;
	}

	delete[] surfacecvs;
}

void KeyUp(KeyCode key)
{
	for (int i = 0; i < ARRAY_SIZE(curves); ++i) {
		if (key == KeyCode1 + i) {
			ChangeCurve(i);
			Tessellate();
		}
	}

	switch (key) {
	case KeyCodeF:
		fullscreen = !fullscreen;
		break;

	case KeyCodeW:
		wireframe = !wireframe;
		break;

	case KeyCodeAdd:
		numsegments = Math::Min<int>(numsegments + 10, MAX_NUM_SEGMENTS);
		Tessellate();
		break;

	case KeyCodeSubtract:
		numsegments = Math::Max<int>(numsegments - 10, 10);
		Tessellate();
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint32_t screenwidth = app->GetClientWidth();
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		if (!fullscreen) {
			if (UpdateControlPoints((float)x, (float)y))
				Tessellate();
		}

		if (selectedpoint == -1) {
			int left = screenwidth - surfvpwidth - 10;
			int right = screenwidth - 10;
			int top = 10;
			int bottom = surfvpheight + 10;

			if (fullscreen ||
				((x >= left && x <= right) &&
				(y >= top && y <= bottom)))
			{
				camera.OrbitRight(Math::DegreesToRadians(dx));
				camera.OrbitUp(Math::DegreesToRadians(dy));
			}
		}
	} else {
		selectedpoint = -1;
	}
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	Math::Matrix world, view, proj;
	Math::Matrix viewproj;

	Math::Vector4 lightdir			= { 0, 1, 0, 0 };
	Math::Vector4 splineviewport	= { 0, 0, (float)screenwidth, (float)screenheight };
	Math::Vector3 eye;

	Math::Vector2 pointsize			= { 10.0f / screenwidth, 10.0f / screenheight };
	Math::Vector2 gridthickness		= { 1.5f / screenwidth, 1.5f / screenheight };
	Math::Vector2 controlthickness	= { 2.0f / screenwidth, 2.0f / screenheight };
	Math::Vector2 splinethickness	= { 3.0f / screenwidth, 3.0f / screenheight };

	Math::Color	gridcolor(0xffdddddd);
	Math::Color	controlcolor(0xff7470ff);
	Math::Color	splinecolor(0xff000000);
	Math::Color	outsidecolor(0.75f, 0.75f, 0.8f, 1);
	Math::Color	insidecolor(1, 0.66f, 0.066f, 1);

	// play with ortho matrix instead of viewport (line thickness remains constant)
	ConvertToSplineViewport(splineviewport[0], splineviewport[1]);
	ConvertToSplineViewport(splineviewport[2], splineviewport[3]);

	// render grid, control polygon and curve
	Math::MatrixIdentity(world);
	Math::MatrixOrthoOffCenterRH(proj, splineviewport[0], splineviewport[2], splineviewport[1], splineviewport[3], -1, 1);

	glViewport(0, 0, screenwidth, screenheight);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	renderlines->SetMatrix("matViewProj", proj);
	renderlines->SetMatrix("matWorld", world);
	renderlines->SetVector("matColor", gridcolor);
	renderlines->SetVector("lineThickness", gridthickness);

	renderlines->Begin();
	{
		supportlines->DrawSubset(0);

		renderlines->SetVector("lineThickness", controlthickness);
		renderlines->SetVector("matColor", controlcolor);
		renderlines->CommitChanges();

		supportlines->DrawSubset(1);

		renderlines->SetVector("lineThickness", splinethickness);
		renderlines->SetVector("matColor", splinecolor);
		renderlines->CommitChanges();

		curve->DrawSubset(0);
	}
	renderlines->End();

	// render control points
	renderpoints->SetMatrix("matViewProj", proj);
	renderpoints->SetMatrix("matWorld", world);
	renderpoints->SetVector("matColor", controlcolor);
	renderpoints->SetVector("pointSize", pointsize);

	renderpoints->Begin();
	{
		glBindVertexArray(supportlines->GetVertexLayout());
		glDrawArrays(GL_POINTS, 44, NumControlVertices);
	}
	renderpoints->End();

	// render surface
	if (!fullscreen) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(screenwidth - surfvpwidth - 10, screenheight - surfvpheight - 10, surfvpwidth, surfvpheight);
	}

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_DEPTH_TEST);

	if (fullscreen)
		glViewport(0, 0, screenwidth, screenheight);
	else
		glViewport(screenwidth - surfvpwidth - 10, screenheight - surfvpheight - 10, surfvpwidth, surfvpheight);

	camera.Animate(alpha);

	if (fullscreen)
		camera.SetAspect((float)screenwidth / screenheight);
	else
		camera.SetAspect((float)surfvpwidth / surfvpheight);

	camera.GetViewMatrix(view);
	camera.GetEyePosition(eye);
	camera.GetProjectionMatrix(proj);

	Math::MatrixMultiply(viewproj, view, proj);

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDisable(GL_CULL_FACE);

	rendersurface->SetMatrix("matViewProj", viewproj);
	rendersurface->SetMatrix("matWorld", world);
	rendersurface->SetMatrix("matWorldInv", world);
	rendersurface->SetVector("lightDir", lightdir);
	rendersurface->SetVector("eyePos", eye);
	rendersurface->SetVector("outsideColor", outsidecolor);
	rendersurface->SetVector("insideColor", insidecolor);
	rendersurface->SetInt("isWireMode", wireframe);

	rendersurface->Begin();
	{
		surface->DrawSubset(0);
	}
	rendersurface->End();

	glEnable(GL_CULL_FACE);

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (!fullscreen) {
		// render text
		glViewport(3, 0, 800, 130);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, helptext);

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
