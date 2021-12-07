
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"

// helper macros
#define TITLE				"Shader sample 51: Tessellation shader"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*	app					= nullptr;

OpenGLMesh*		gridmesh			= nullptr;
OpenGLEffect*	drawcurveeffect		= nullptr;
OpenGLEffect*	drawgrideffect		= nullptr;
OpenGLEffect*	drawpointseffect	= nullptr;
OpenGLEffect*	drawlineseffect		= nullptr;

GLuint			controlpointVBO		= 0;
GLuint			controltangentVBO	= 0;
GLuint			controlpointVAO		= 0;
GLuint			controltangentVAO	= 0;

int				selectedpoint		= -1;
float			selectiondx			= 0;
float			selectiondy			= 0;

// NOTE: for 1360x768
Math::Vector4 controlpoints[] = {
	{ 200, 100, 0, 0 },
	{ 222, 407, 0, 0 },
	{ 315, 587, 0, 0 },
	{ 684, 304, 0, 0 },

	{ 963, 387, 0, 0 },
	{ 1090, 615, 0, 0 },
	{ 671, 688, 0, 0 },
	{ 710, 507, 0, 0 }
};

Math::Vector4 controltangents[] = {
	{ 127, 162, 0, 0 },
	{ -149, 71, 0, 0 },
	{ 161, 35, 0, 0 },
	{ 135, -89, 0, 0 },

	{ 163, -49, 0, 0 },
	{ -118, 99, 0, 0 },
	{ -95, -69, 0, 0 },
	{ 108, -12, 0, 0 }
};

void ConvertPointsToPatches();

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(1, 1, 1, 1);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDisable(GL_DEPTH_TEST);

	// scale control points to screen resolution
	float scalex = screenwidth / 1360.0f;
	float scaley = screenheight / 768.0f;

	for (int i = 0; i < ARRAY_SIZE(controlpoints); ++i) {
		controlpoints[i][0] *= scalex;
		controlpoints[i][1] *= scaley;

		controltangents[i][0] *= scalex;
		controltangents[i][1] *= scaley;
	}

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/drawgrid.vert", 0, 0, 0, "../../Media/ShadersGL/drawcurve.frag", &drawgrideffect)) {
		MYERROR("Could not create 'grid' effect");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/drawgrid.vert", 0, 0, "../../Media/ShadersGL/drawpoints.geom", "../../Media/ShadersGL/drawcurve.frag", &drawpointseffect)) {
		MYERROR("Could not create 'points' effect");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/drawlines.vert", 0, 0, 0, "../../Media/ShadersGL/drawcurve.frag", &drawlineseffect)) {
		MYERROR("Could not create 'lines' effect");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/drawcurve.vert", "../../Media/ShadersGL/drawcurve.tesc", "../../Media/ShadersGL/drawcurve.tese", "../../Media/ShadersGL/drawlines.geom", "../../Media/ShadersGL/drawcurve.frag", &drawcurveeffect)) {
		MYERROR("Could not create tessellation program");
		return false;
	}

	// control points
	glGenBuffers(1, &controlpointVBO);
	
	glBindBuffer(GL_ARRAY_BUFFER, controlpointVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(controlpoints) * 2 - 2 * sizeof(Math::Vector4), NULL, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &controlpointVAO);
	glBindVertexArray(controlpointVAO);
	{
		glBindBuffer(GL_ARRAY_BUFFER, controlpointVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Math::Vector4), (const void*)0);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// control tangents
	glGenBuffers(1, &controltangentVBO);

	glBindBuffer(GL_ARRAY_BUFFER, controltangentVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(controltangents), NULL, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &controltangentVAO);
	glBindVertexArray(controltangentVAO);
	{
		glBindBuffer(GL_ARRAY_BUFFER, controltangentVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Math::Vector4), (const void*)0);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ConvertPointsToPatches();

	// create grid
	OpenGLVertexElement decl[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	float (*vdata)[3] = 0;
	GLushort* idata = 0;
	GLuint gridlinesx = (screenwidth / 16) - 1;
	GLuint gridlinesy = (screenheight / 16) - 1;

	if (!GLCreateMesh((gridlinesx + gridlinesy) * 2, 0, 0, decl, &gridmesh)) {
		MYERROR("Could not create grid mesh");
		return false;
	}

	gridmesh->LockVertexBuffer(0, 0, GL_MAP_WRITE_BIT, (void**)&vdata);
	{
		for (GLuint i = 0; i < gridlinesx * 2; i += 2) {
			vdata[i][0] = (i / 2 + 1) * 16.0f;
			vdata[i][1] = 0;
			vdata[i][2] = 0;

			vdata[i + 1][0] = (i / 2 + 1) * 16.0f;
			vdata[i + 1][1] = (float)screenheight;
			vdata[i + 1][2] = 0;
		}

		vdata += (gridlinesx * 2);

		for (GLuint i = 0; i < gridlinesy * 2; i += 2) {
			vdata[i][0] = 0;
			vdata[i][1] = (i / 2 + 1) * 16.0f;
			vdata[i][2] = 0;

			vdata[i + 1][0] = (float)screenwidth;
			vdata[i + 1][1] = (i / 2 + 1) * 16.0f;
			vdata[i + 1][2] = 0;
		}
	}
	gridmesh->UnlockVertexBuffer();

	gridmesh->GetAttributeTable()[0].PrimitiveType	= GL_LINES;
	gridmesh->GetAttributeTable()[0].VertexStart	= 0;
	gridmesh->GetAttributeTable()[0].VertexCount	= gridmesh->GetNumVertices();

	return true;
}

void ConvertPointsToPatches()
{
	int numpoints = ARRAY_SIZE(controlpoints);
	int numverts = numpoints * 2 - 2;

	glBindBuffer(GL_ARRAY_BUFFER, controlpointVBO);
	Math::Vector4* vdata = (Math::Vector4*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	{
		// first and last are not duplicated
		vdata[0] = controlpoints[0];
		vdata[numverts - 1] = controlpoints[numpoints - 1];

		// duplicate others
		for (int i = 1, j = 1; i < numpoints - 1; ++i, j += 2) {
			vdata[j] = controlpoints[i];
			vdata[j + 1] = controlpoints[i];
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// update tangents
	glBindBuffer(GL_ARRAY_BUFFER, controltangentVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(controltangents), controltangents);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UninitScene()
{
	delete drawcurveeffect;
	delete drawpointseffect;
	delete drawlineseffect;
	delete drawgrideffect;
	delete gridmesh;

	glDeleteVertexArrays(1, &controlpointVAO);
	glDeleteVertexArrays(1, &controltangentVAO);

	glDeleteBuffers(1, &controlpointVBO);
	glDeleteBuffers(1, &controltangentVBO);

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
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();
	uint8_t state = app->GetMouseButtonState();

	float radius = 15.0f;
	float dist;
	float mx = (float)x;
	float my = (float)(screenheight - y - 1);
	int numpoints = ARRAY_SIZE(controlpoints);

	if (state & MouseButtonLeft) {
		if (selectedpoint == -1) {
			// look for a point
			for (int i = 0; i < numpoints; ++i) {
				selectiondx = controlpoints[i][0] - mx;
				selectiondy = controlpoints[i][1] - my;
				dist = selectiondx * selectiondx + selectiondy * selectiondy;

				if (dist < radius * radius) {
					selectedpoint = i;
					break;
				}
			}
		}

		if (selectedpoint == -1) {
			// look for a tangent
			for (int i = 0; i < numpoints; ++i) {
				selectiondx = (controlpoints[i][0] + controltangents[i][0]) - mx;
				selectiondy = (controlpoints[i][1] + controltangents[i][1]) - my;
				dist = selectiondx * selectiondx + selectiondy * selectiondy;

				if (dist < radius * radius) {
					selectedpoint = numpoints + i;
					break;
				}
			}
		}

		if (selectedpoint > -1 && selectedpoint < numpoints) {
			// a point is selected
			controlpoints[selectedpoint][0] = Math::Min<float>(Math::Max<float>(selectiondx + mx, 0), (float)screenwidth - 1);
			controlpoints[selectedpoint][1] = Math::Min<float>(Math::Max<float>(selectiondy + my, 0), (float)screenheight - 1);

			ConvertPointsToPatches();
		} else if (selectedpoint >= numpoints && selectedpoint < numpoints * 2) {
			// a tangent is selected
			int selectedtangent = selectedpoint - numpoints;

			//float tpx = controlpoints[selectedtangent][0] + tangents[selectedtangent][0];
			//float tpy = controlpoints[selectedtangent][1] + tangents[selectedtangent][1];

			float tpx = Math::Min<float>(Math::Max<float>(selectiondx + mx, 0), (float)screenwidth - 1);
			float tpy = Math::Min<float>(Math::Max<float>(selectiondy + my, 0), (float)screenheight - 1);

			controltangents[selectedtangent][0] = tpx - controlpoints[selectedtangent][0];
			controltangents[selectedtangent][1] = tpy - controlpoints[selectedtangent][1];

			ConvertPointsToPatches();
		}
	} else {
		selectedpoint = -1;
	}
}

void Update(float delta)
{
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix	proj;
	Math::Color		black			= { 0, 0, 0, 1 };
	Math::Color		grey			= { 0.75f, 0.75f, 0.75f, 1 };
	Math::Color		pointcolor(0xff7470ff);
	Math::Color		tangentcolor(0xff74dd70);

	uint32_t		screenwidth		= app->GetClientWidth();
	uint32_t		screenheight	= app->GetClientHeight();

	Math::Vector2	pointsize		= { 10.0f / screenwidth, 10.0f / screenheight };
	Math::Vector2	curvethickness	= { 3.0f / screenwidth, 3.0f / screenheight };
	
	GLuint			funcindex		= 0;
	GLsizei			numpoints		= ARRAY_SIZE(controlpoints);
	GLsizei			numvertices		= numpoints * 2 - 2;

	glClear(GL_COLOR_BUFFER_BIT);
	glPatchParameteri(GL_PATCH_VERTICES, 2);

	Math::MatrixOrthoOffCenterRH(proj, 0, (float)screenwidth, 0, (float)screenheight, -1, 1);

	// render grid
	drawgrideffect->SetMatrix("matProj", proj);
	drawgrideffect->SetVector("color", &grey.r);
	drawgrideffect->Begin();
	{
		funcindex = 0;
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &funcindex);

		gridmesh->DrawSubset(0);
	}
	drawgrideffect->End();

	// render tangent lines (with a cool shader trick)
	drawlineseffect->SetMatrix("matProj", proj);
	drawlineseffect->SetVector("color", &tangentcolor.r);
	drawlineseffect->SetInt("numControlVertices", numvertices);
	drawlineseffect->Begin();
	{
		glBindVertexArray(controlpointVAO);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, controlpointVBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, controltangentVBO);

		glDrawArrays(GL_LINES, 0, numpoints * 2);
	}
	drawlineseffect->End();

	// render spline
	drawcurveeffect->SetMatrix("matProj", proj);
	drawcurveeffect->SetVector("lineThickness", curvethickness);
	drawcurveeffect->SetVector("color", &black.r);
	drawcurveeffect->Begin();
	{
		glBindVertexArray(controlpointVAO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, controltangentVBO);

		glDrawArrays(GL_PATCHES, 0, numvertices);
	}
	drawcurveeffect->End();

	// render control points
	drawpointseffect->SetMatrix("matProj", proj);
	drawpointseffect->SetVector("pointSize", pointsize);
	drawpointseffect->SetVector("color", &pointcolor.r);
	drawpointseffect->Begin();
	{
		funcindex = 0;
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &funcindex);

		glBindVertexArray(controlpointVAO);
		glDrawArrays(GL_POINTS, 0, numvertices);
	}
	drawpointseffect->End();

	// render tangent points
	drawpointseffect->SetVector("color", &tangentcolor.r);
	drawpointseffect->Begin();
	{
		funcindex = 1;
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &funcindex);
		glUniform1i(1, numvertices);

		glBindVertexArray(controltangentVAO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, controlpointVBO);

		glDrawArrays(GL_POINTS, 0, numpoints);
	}
	drawpointseffect->End();

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
