
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define DEFAULT_DISTANCE	5.0f

// helper macros
#define TITLE				"Shader sample 51: Shader based anti-aliasing"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample classes
class LineRenderer
{
	struct LineVertex
	{
		Math::Vector4 pos;
		Math::Vector2 segment0;	// segment before joint
		Math::Vector2 segment1;	// segment after joint
		uint32_t color;
		float thickness;
		float linelength;		// total length
	};

	static_assert(sizeof(LineVertex) == 44, "sanity check");

private:
	std::vector<LineVertex> vertices;
	std::vector<uint32_t> indices;

	GLuint vertexbuffer;
	GLuint indexbuffer;
	GLuint inputlayout;

	Math::Color linecolor;
	Math::Vector2 pointer;
	float linewidth;
	bool wasmoveto;

public:
	LineRenderer();
	~LineRenderer();

	void Begin();
	void End();

	void MoveTo(float x, float y);
	void LineTo(float x, float y);
	void SetColor(const Math::Color& color);
	void SetLineWidth(float width);
};

LineRenderer::LineRenderer()
{
	vertexbuffer	= 0;
	indexbuffer		= 0;
	inputlayout		= 0;

	linewidth		= 1;
	linecolor		= { 1, 1, 1, 1 };
	wasmoveto		= true;
}

LineRenderer::~LineRenderer()
{
	glDeleteVertexArrays(1, &inputlayout);

	GL_SAFE_DELETE_BUFFER(vertexbuffer);
	GL_SAFE_DELETE_BUFFER(indexbuffer);
}

void LineRenderer::Begin()
{
	vertices.clear();
	indices.clear();
}

void LineRenderer::End()
{
	if (inputlayout == 0) {
		glGenBuffers(1, &vertexbuffer);
		glGenBuffers(1, &indexbuffer);
		glGenVertexArrays(1, &inputlayout);

		glBindVertexArray(inputlayout);
		{
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)16);

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)24);

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(LineVertex), (void*)32);

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)36);

			glEnableVertexAttribArray(5);
			glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)40);
		}
		glBindVertexArray(0);
	}

	assert(vertices.size() > 1);

	// generate vertices (complexity: O(n))
	float totallength = 0;
	size_t startvertex = 0;
	size_t numshapes = 1;

	auto closeShape = [&](size_t start, size_t end) {
		if (end - start > 1) {
			LineVertex& first = vertices[start];
			LineVertex& last = vertices[end];

			Math::Vector3 diff = last.pos - first.pos;
					
			if (Math::Vec3Dot(diff, diff) < 1e-8f) {
				// connect first and last vertex
				last.segment1 = first.segment1;
				first.segment0 = last.segment0;

				// push them out from [0, totallength]
				last.pos.w -= Math::Vec2Length(last.segment0) * 0.5f;
				first.pos.w += Math::Vec2Length(first.segment1) * 0.5f;
			}
		}
	};

	for (size_t i = 0; i < vertices.size(); ++i) {
		LineVertex& curr = vertices[i];
		
		if (i > startvertex) {
			// any vertex except first
			const LineVertex& prev = vertices[i - 1];

			curr.segment0 = { curr.pos.x - prev.pos.x, curr.pos.y - prev.pos.y };
			totallength += Math::Vec2Length(curr.segment0);

			curr.pos.w = totallength;
		}

		if (i < vertices.size() - 1) {
			// any vertex except last
			const LineVertex& next = vertices[i + 1];
			
			if (next.pos.w < 0) {
				// next is shape terminator (segment1 provokes the quad)
				curr.segment1 = curr.segment0;
				curr.pos.w = totallength;

				for (size_t j = startvertex; j <= i; ++j)
					vertices[j].linelength = totallength;

				closeShape(startvertex, i);

				startvertex = i + 1;
				totallength = 0;

				++numshapes;
			} else {
				curr.segment1 = { next.pos.x - curr.pos.x, next.pos.y - curr.pos.y };
				curr.pos.w = totallength;
			}
		} else {
			// very last vertex
			for (size_t j = startvertex; j <= i; ++j)
				vertices[j].linelength = totallength;

			closeShape(startvertex, i);
		}
	}

	// expand to quads
	vertices.resize(vertices.size() * 2);

	for (int i = (int)vertices.size() - 1; i >= 1 ; i -= 2) {
		vertices[i] = vertices[i >> 1];
		vertices[i - 1] = vertices[i >> 1];
	}

	// generate indices
	uint32_t vertexoffset = 0;
	indices.resize((vertices.size() / 2 - numshapes) * 6);

	for (size_t i = 0; i < indices.size(); i += 6) {
		const LineVertex& next = vertices[vertexoffset + 2];

		if (next.pos.w * next.pos.w < 0.25001f * Math::Vec2Dot(next.segment1, next.segment1))
			vertexoffset += 2;

		indices[i + 0] = vertexoffset + 0;
		indices[i + 1] = vertexoffset + 1;
		indices[i + 2] = vertexoffset + 3;

		indices[i + 3] = vertexoffset + 0;
		indices[i + 4] = vertexoffset + 3;
		indices[i + 5] = vertexoffset + 2;

		vertexoffset += 2;
	}

	// upload to GPU
	GLint size = 0;

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	if (size < vertices.size() * sizeof(LineVertex))
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
	else
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(LineVertex), vertices.data());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	if (size < indices.size() * sizeof(uint32_t))
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
	else
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());

	// render
	glBindVertexArray(inputlayout);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

	glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}

void LineRenderer::MoveTo(float x, float y)
{
	pointer = { x, y };
	wasmoveto = true;
}

void LineRenderer::LineTo(float x, float y)
{
	LineVertex vert;

	if (wasmoveto) {
		vert.pos = Math::Vector4(pointer.x, pointer.y, 0, -1);
		vert.color = linecolor;
		vert.thickness = linewidth;

		vertices.push_back(vert);
	}

	vert.pos = Math::Vector4(x, y, 0, 1);
	vert.color = linecolor;
	vert.thickness = linewidth;

	vertices.push_back(vert);
	wasmoveto = false;
}

void LineRenderer::SetColor(const Math::Color& color)
{
	linecolor = color;
}

void LineRenderer::SetLineWidth(float width)
{
	linewidth = width;
}

// sample variables
Application*		app				= nullptr;
LineRenderer*		renderer		= nullptr;

OpenGLEffect*		renderline		= nullptr;
OpenGLScreenQuad*	screenquad		= nullptr;

BasicCamera			camera;
GLuint				helptext		= 0;

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

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/shaderline.vert", 0, 0, 0, "../../Media/ShadersGL/shaderline.frag", &renderline))
		return false;

	renderer = new LineRenderer();
	screenquad = new OpenGLScreenQuad();

	camera.SetDistance(DEFAULT_DISTANCE);
	camera.SetZoomLimits(1, DEFAULT_DISTANCE);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Middle mouse - Pan/zoom",
		helptext, 512, 512);

	return true;
}

void UninitScene()
{
	delete renderer;
	delete renderline;
	delete screenquad;

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

	if (state & MouseButtonMiddle) {
		camera.PanRight(dx * -1.0f);
		camera.PanUp(dy * -1.0f);
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
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	Math::Matrix world, proj;
	Math::Vector3 eye;

	camera.Animate(alpha);
	camera.GetEyePosition(eye);

	float scale = camera.GetDistance() / DEFAULT_DISTANCE;
	float halfwidth = 0.5f * screenwidth;
	float halfheight = 0.5f * screenheight;

	// M = (center^(-1) * scaling * center) * translation
	float left = -halfwidth * scale + (eye.x + halfwidth);
	float bottom = -halfheight * scale + (eye.y + halfheight);

	Math::MatrixOrthoOffCenterRH(proj, left, left + scale * screenwidth, bottom + scale * screenheight, bottom, -1, 1);

	// render
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	renderer->Begin();
	
	// single line
	renderer->SetColor(Math::Color(1, 1, 1, 1));
	renderer->SetLineWidth(5);

	renderer->MoveTo(screenwidth * 0.05f, screenheight * 0.5f);
	renderer->LineTo(screenwidth * 0.2f, screenheight * 0.1f);

	// open angle
	Math::Vector2 p1(screenwidth * 0.3f, screenheight * 0.13f);
	Math::Vector2 p2(screenwidth * 0.5f, screenheight * 0.13f);
	Math::Vector2 p3;
	float angle = Math::DegreesToRadians(3);
	float length = Math::Vec2Distance(p1, p2);

	p3.x = p1.x + cosf(angle) * length;
	p3.y = p1.y + sinf(angle) * length;

	renderer->SetColor(Math::Color(1, 0, 1, 1));
	renderer->MoveTo(p2.x, p2.y);
	renderer->LineTo(p1.x, p1.y);
	renderer->LineTo(p3.x, p3.y);

	// mutliple lines
	renderer->SetColor(Math::Color(0, 0, 1, 1));
	renderer->SetLineWidth(7);

	renderer->MoveTo(screenwidth * 0.15f, screenheight * 0.8f);
	renderer->LineTo(screenwidth * 0.35f, screenheight * 0.3f);
	renderer->LineTo(screenwidth * 0.45f, screenheight * 0.3f);
	renderer->LineTo(screenwidth * 0.65f, screenheight * 0.8f);
	
	// triangle
	renderer->SetColor(Math::Color(0, 1, 0, 1));
	renderer->SetLineWidth(10);

	renderer->MoveTo(screenwidth * 0.3f, screenheight * 0.75f);
	renderer->LineTo(screenwidth * 0.5f, screenheight * 0.75f);
	renderer->LineTo(screenwidth * 0.4f, screenheight * 0.5f);
	renderer->LineTo(screenwidth * 0.3f, screenheight * 0.75f);

	// star
	renderer->SetColor(Math::Color(0, 0.5f, 1, 1));
	renderer->SetLineWidth(20);

	Math::Vector2 center(screenwidth * 0.8f, screenheight * 0.4f);
	uint16_t segments = 16;
	float step = Math::TWO_PI / segments;
	float largesize = screenheight * 0.3f;
	float smallsize = screenheight * 0.15f;

	renderer->MoveTo(center.x + cosf(0.0f) * largesize, center.y + sinf(0.0f) * largesize);

	for (uint16_t i = 1; i <= segments; ++i) {
		if (i % 2 == 0)
			renderer->LineTo(center.x + cosf(step * i) * largesize, center.y + sinf(step * i) * largesize);
		else
			renderer->LineTo(center.x + cosf(step * i) * smallsize, center.y + sinf(step * i) * smallsize);
	}

	// draw
	renderline->SetMatrix("matProj", &proj._11);
	renderline->SetFloat("smoothingLevel", 1.5f * scale);

	renderline->Begin();
	{
		renderer->End();
	}
	renderline->End();

	// render text
	glViewport(10, screenheight - 522, 512, 512);

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
	Math::Vector2 n, n0, n1;
	const int maxn = 10000;

	for (int i = 0; i <= maxn; ++i) {
		float angle = ((float)i / (float)maxn) * Math::HALF_PI;

		n0.x = cos(angle);
		n0.y = sin(angle);

		n1.x = cos(angle);
		n1.y = -sin(angle);

		Math::Vec2Normalize(n, n0 + n1);

		float cosangle = Math::Vec2Dot(n0, n1);
		float test = 1.0f / Math::Vec2Dot(n, n1);

		if (std::isnan(n.x) || std::isnan(n.y) || std::isnan(test)) {
			printf("NaN at %f\n", angle);
		}
	}

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
