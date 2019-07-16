
#include "drawingitem.h"
#include "renderingcore.h"

// --- NativeContext impl -----------------------------------------------------

class NativeContext::FlushPrimitivesTask : public IRenderingTask
{
public:
	typedef std::vector<Math::Vector4> VertexList;
	typedef std::vector<uint16_t> IndexList;

private:
	Math::Matrix		world;
	Math::Color			clearcolor;
	Math::Color			linecolor;
	VertexList			vertices;
	IndexList			indices;
	OpenGLFramebuffer*	rendertarget;	// external
	OpenGLMesh*			mesh;
	OpenGLEffect*		lineeffect;
	bool				needsclear;

	void Dispose() override
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(lineeffect);
		SAFE_DELETE(mesh);
	}

	void Execute(IRenderingContext* context) override
	{
		// NOTE: runs on renderer thread
		OpenGLVertexElement decl[] = {
			{ 0, 0, GLDECLTYPE_FLOAT4, GLDECLUSAGE_POSITION, 0 },
			{ 0xff, 0, 0, 0, 0 }
		};

		OpenGLAttributeRange table[] = {
			{ GLPT_LINELIST, 0, 0, (GLuint)indices.size(), 0, (GLuint)vertices.size(), true }
		};

		float			width		= (float)rendertarget->GetWidth();
		float			height		= (float)rendertarget->GetHeight();
		Math::Matrix	proj;
		Math::Vector2	thickness	= { 3.0f / width, 3.0f / height };
		void*			vdata		= nullptr;
		void*			idata		= nullptr;

		if (lineeffect == nullptr) {
			lineeffect = context->CreateEffect(
				"../../Media/ShadersGL/simplecolor.vert",
				"../../Media/ShadersGL/drawlines.geom",		// NOTE: causes driver crash on Intel
				"../../Media/ShadersGL/simplecolor.frag");
		}

		if (mesh == nullptr)
			mesh = context->CreateMesh(128, 256, GLMESH_DYNAMIC, decl);

		mesh->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
		mesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, &idata);

		memcpy(vdata, vertices.data(), vertices.size() * sizeof(Math::Vector4));
		memcpy(idata, indices.data(), indices.size() * sizeof(GLushort));

		mesh->UnlockIndexBuffer();
		mesh->UnlockVertexBuffer();
		mesh->SetAttributeTable(table, 1);

		Math::MatrixOrthoOffCenterRH(proj, width * -0.5f, width * 0.5f, height * -0.5f, height * 0.5f, -1, 1);

		lineeffect->SetMatrix("matWorld", world);
		lineeffect->SetMatrix("matViewProj", proj);
		lineeffect->SetVector("matColor", &linecolor.r);
		lineeffect->SetVector("lineThickness", thickness);

		rendertarget->Set();
		{
			if (needsclear)
				context->Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, clearcolor);

			lineeffect->Begin();
			{
				mesh->DrawSubset(0);
			}
			lineeffect->End();
		}
		rendertarget->Unset();

		needsclear = false;
	}

public:
	FlushPrimitivesTask(int universe, OpenGLFramebuffer* framebuffer)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		rendertarget	= framebuffer;
		lineeffect		= nullptr;
		mesh			= nullptr;

		clearcolor		= Math::Color(1, 1, 1, 1);
		linecolor		= Math::Color(1, 1, 1, 1);
		needsclear		= false;

		Math::MatrixIdentity(world);
	}

	void SetNeedsClear(const Math::Color& color)
	{
		// NOTE: runs on any other thread
		needsclear = true;
		clearcolor = color;
	}

	void SetData(const VertexList& verts, const IndexList& inds)
	{
		// NOTE: runs on any other thread
		vertices = verts;
		indices = inds;
	}

	void SetLineColor(const Math::Color& color)
	{
		// NOTE: runs on any other thread
		linecolor = color;
	}

	void SetWorldMatrix(const Math::Matrix& matrix)
	{
		// NOTE: runs on any other thread
		world = matrix;
	}
};

NativeContext::NativeContext()
{
	owneritem	= nullptr;
	ownerlayer	= nullptr;
	flushtask	= nullptr;
}

NativeContext::NativeContext(const NativeContext& other)
{
	owneritem	= nullptr;
	ownerlayer	= nullptr;
	flushtask	= nullptr;

	operator =(other);
}

NativeContext::NativeContext(DrawingItem* item, DrawingLayer* layer)
{
	owneritem = item;
	ownerlayer = layer;

	flushtask = new FlushPrimitivesTask(item->GetOpenGLUniverseID(), layer->GetRenderTarget());

	vertices.reserve(128);
	indices.reserve(256);
}

NativeContext::~NativeContext()
{
	FlushPrimitives();

	if (ownerlayer != nullptr)
		ownerlayer->contextguard.unlock();
}

void NativeContext::FlushPrimitives()
{
	if (flushtask != nullptr) {
		flushtask->SetData(vertices, indices);
		flushtask->MarkForDispose();

		GetRenderingCore()->AddTask(flushtask);
		flushtask = nullptr;
	}

	vertices.clear();
	indices.clear();
}

void NativeContext::Clear(const Math::Color& color)
{
	if (flushtask != nullptr)
		flushtask->SetNeedsClear(color);
}

void NativeContext::SetColor(const Math::Color& color)
{
	if (flushtask != nullptr)
		flushtask->SetLineColor(color);
}

void NativeContext::MoveTo(float x, float y)
{
	if ((indices.size() > 0 && vertices.size() + 1 > indices.size()) ||
		(indices.size() == 0 && vertices.size() == 1))
	{
		// detected 2 MoveTo()s in a row
		vertices.pop_back();
	}

	vertices.push_back(Math::Vector4(x, y, 0, 1));
}

void NativeContext::LineTo(float x, float y)
{
	if (vertices.size() == 0)
		vertices.push_back(Math::Vector4(0, 0, 0, 1));

	indices.push_back((uint16_t)vertices.size() - 1);
	indices.push_back((uint16_t)vertices.size());

	vertices.push_back(Math::Vector4(x, y, 0, 1));
}

void NativeContext::SetWorldTransform(const Math::Matrix& transform)
{
	if (flushtask)
		flushtask->SetWorldMatrix(transform);
}

NativeContext& NativeContext::operator =(const NativeContext& other)
{
	// this is a forced move semantic
	if (&other == this)
		return *this;

	FlushPrimitives();

	flushtask = other.flushtask;
	owneritem = other.owneritem;
	ownerlayer = other.ownerlayer;

	other.flushtask = nullptr;
	other.owneritem = nullptr;
	other.ownerlayer = nullptr;

	return *this;
}

// --- DrawingLayer impl ------------------------------------------------------

class DrawingLayer::DrawingLayerSetupTask : public IRenderingTask
{
private:
	OpenGLFramebuffer*	rendertarget;
	GLuint				rtwidth, rtheight;

	void Dispose() override
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(rendertarget);
	}

	void Execute(IRenderingContext* context) override
	{
		// NOTE: runs on renderer thread
		if (rendertarget == nullptr || IsMarkedForDispose()) {
			rendertarget = context->CreateFramebuffer(rtwidth, rtheight);

			rendertarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8B8G8R8, GL_LINEAR);
			rendertarget->AttachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GLFMT_D24S8);
		}
	}

public:
	DrawingLayerSetupTask(int universe, GLuint width, GLuint height)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		rendertarget	= nullptr;
		rtwidth			= width;
		rtheight		= height;
	}

	inline OpenGLFramebuffer* GetRenderTarget() const	{ return rendertarget; }
};

DrawingLayer::DrawingLayer(int universe, uint32_t width, uint32_t height)
{
	owner = nullptr;
	setuptask = new DrawingLayerSetupTask(universe, width, height);
	
	GetRenderingCore()->AddTask(setuptask);
	GetRenderingCore()->Barrier();
}

DrawingLayer::~DrawingLayer()
{
	setuptask->MarkForDispose();
	GetRenderingCore()->AddTask(setuptask);

	setuptask = nullptr;
}

OpenGLFramebuffer* DrawingLayer::GetRenderTarget() const
{
	return setuptask->GetRenderTarget();
}

NativeContext DrawingLayer::GetContext()
{
	contextguard.lock();
	return NativeContext(owner, this);
}

// --- DrawingItem impl -------------------------------------------------------

class DrawingItem::RecomposeLayersTask : public IRenderingTask
{
private:
	DrawingItem*		item;
	OpenGLScreenQuad*	screenquad;

	void Dispose() override
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(screenquad);
	}

	void Execute(IRenderingContext* context) override
	{
		// NOTE: runs on renderer thread
		if (item == nullptr || IsMarkedForDispose())
			return;

		if (screenquad == nullptr) {
			Math::Matrix world(1, 1, 1, 1);

			screenquad = context->CreateScreenQuad();
			screenquad->SetTextureMatrix(world);
		}

		if (screenquad != nullptr) {
			GLuint texid = item->GetBottomLayer().GetRenderTarget()->GetColorAttachment(0);

			context->SetDepthTest(GL_FALSE);
			context->Clear(GL_COLOR_BUFFER_BIT, Math::Color(0, 0, 0, 1));

			glBindTexture(GL_TEXTURE_2D, texid);
			screenquad->Draw();

			texid = item->GetFeedbackLayer().GetRenderTarget()->GetColorAttachment(0);
			glBindTexture(GL_TEXTURE_2D, texid);

			context->SetBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			screenquad->Draw();

			context->SetBlendMode(GL_NONE, GL_NONE);
			context->SetDepthTest(GL_TRUE);
		}

		context->Present(universeid);
	}

public:
	RecomposeLayersTask(int universe, DrawingItem* drawingitem)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		screenquad	= nullptr;
		item		= drawingitem;
	}
};

DrawingItem::DrawingItem(int universe, uint32_t width, uint32_t height)
	: bottomlayer(universe, width, height), feedbacklayer(universe, width, height)
{
	universeID = universe;
	recomposetask = new RecomposeLayersTask(universeID, this);

	// DON'T USE 'this' in initializer list!!!
	bottomlayer.owner = this;
	feedbacklayer.owner = this;
}

DrawingItem::~DrawingItem()
{
	recomposetask->MarkForDispose();
	GetRenderingCore()->AddTask(recomposetask);

	recomposetask = nullptr;
}

void DrawingItem::RecomposeLayers()
{
	GetRenderingCore()->AddTask(recomposetask);
	GetRenderingCore()->Barrier();
}
