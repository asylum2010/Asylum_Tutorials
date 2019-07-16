
#ifndef _DRAWINGITEM_H_
#define _DRAWINGITEM_H_

#include <vector>
#include <mutex>

#include "../Common/3DMath.h"

class DrawingItem;
class DrawingLayer;
class OpenGLFramebuffer;
class OpenGLScreenQuad;

/**
 * \brief Interface for 2D rendering
 *
 * Copy constructor and operator = assumes move semantic.
 */
class NativeContext
{
	friend class DrawingLayer;
	class FlushPrimitivesTask;

	typedef std::vector<Math::Vector4> VertexList;
	typedef std::vector<uint16_t> IndexList;

private:
	mutable DrawingItem*			owneritem;
	mutable DrawingLayer*			ownerlayer;
	mutable FlushPrimitivesTask*	flushtask;

	VertexList	vertices;
	IndexList	indices;

	NativeContext();
	NativeContext(DrawingItem* item, DrawingLayer* layer);

	void FlushPrimitives();

public:
	NativeContext(const NativeContext& other);
	~NativeContext();

	void Clear(const Math::Color& color);
	void MoveTo(float x, float y);
	void LineTo(float x, float y);
	void SetWorldTransform(const Math::Matrix& transform);
	void SetColor(const Math::Color& color);

	NativeContext& operator =(const NativeContext& other);
};

/**
 * \brief One layer with attached rendertarget
 */
class DrawingLayer
{
	friend class DrawingItem;
	friend class NativeContext;

	class DrawingLayerSetupTask;

private:
	DrawingItem*			owner;
	DrawingLayerSetupTask*	setuptask;
	std::mutex				contextguard;

	DrawingLayer(int universe, uint32_t width, uint32_t height);
	~DrawingLayer();

public:
	NativeContext GetContext();
	OpenGLFramebuffer* GetRenderTarget() const;
};

/**
 * \brief Multilayered drawing sheet
 */
class DrawingItem
{
	class RecomposeLayersTask;

private:
	DrawingLayer			bottomlayer;
	DrawingLayer			feedbacklayer;
	RecomposeLayersTask*	recomposetask;
	int						universeID;

public:
	DrawingItem(int universe, uint32_t width, uint32_t height);
	~DrawingItem();

	void RecomposeLayers();

	inline DrawingLayer& GetBottomLayer()	{ return bottomlayer; }
	inline DrawingLayer& GetFeedbackLayer()	{ return feedbacklayer; }
	inline int GetOpenGLUniverseID() const	{ return universeID; }
};

#endif
