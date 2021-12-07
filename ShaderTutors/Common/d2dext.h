
#ifndef _D2DEXT_H_
#define _D2DEXT_H_

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <cstdint>
#include <type_traits>
#include <vector>
#include <functional>

#include "3Dmath.h"

#ifdef DIRECT3D11
#	include <d3d11_1.h>
#	include <d3dcompiler.h>
#endif

class OutlineTextRenderer : public IDWriteTextRenderer
{
private:
	ID2D1Factory*			d2dfactory;
	ID2D1RenderTarget*		rendertarget;
	ID2D1SolidColorBrush*	blackbrush;
	ID2D1SolidColorBrush*	whitebrush;
	ID2D1StrokeStyle*		strokestyle;

	// IUnknown methods
	STDMETHOD(QueryInterface)(
		REFIID riid,
		void** ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef(void) override;
	ULONG STDMETHODCALLTYPE Release(void) override;

public:
	OutlineTextRenderer(ID2D1Factory* factory, ID2D1RenderTarget* target);
	~OutlineTextRenderer();

	// IDWritePixelSnapping methods
	STDMETHOD(IsPixelSnappingDisabled)(
		void* clientDrawingContext,
		BOOL* isDisabled) override;

	STDMETHOD(GetCurrentTransform)(
		void* clientDrawingContext,
		DWRITE_MATRIX* transform) override;

	STDMETHOD(GetPixelsPerDip)(
		void* clientDrawingContext,
		FLOAT* pixelsPerDip) override;

	// IDWriteTextRenderer methods
	STDMETHOD(DrawGlyphRun)(
		void* clientDrawingContext,
		FLOAT baselineOriginX,
		FLOAT baselineOriginY,
		DWRITE_MEASURING_MODE measuringMode,
		DWRITE_GLYPH_RUN const* glyphRun,
		DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
		IUnknown* clientDrawingEffect) override;

	STDMETHOD(DrawUnderline)(
		void* clientDrawingContext,
		FLOAT baselineOriginX,
		FLOAT baselineOriginY,
		DWRITE_UNDERLINE const* underline,
		IUnknown* clientDrawingEffect) override;

	STDMETHOD(DrawStrikethrough)(
		void* clientDrawingContext,
		FLOAT baselineOriginX,
		FLOAT baselineOriginY,
		DWRITE_STRIKETHROUGH const* strikethrough,
		IUnknown* clientDrawingEffect) override;

	STDMETHOD(DrawInlineObject)(
		void* clientDrawingContext,
		FLOAT originX,
		FLOAT originY,
		IDWriteInlineObject* inlineObject,
		BOOL isSideways,
		BOOL isRightToLeft,
		IUnknown* clientDrawingEffect) override;
};

#ifdef DIRECT3D11
class D3D11Brush
{
protected:
	D3D11Brush() {}

public:
	virtual ~D3D11Brush() {};
};

class D3D11InstancedRenderer : public ID2D1TessellationSink
{
	struct CustomVertex
	{
		float x, y, z;
		uint32_t color;
		uint32_t instID;

		CustomVertex() {
			x = y = z = 0;
			color = 0xff000000;
		}

		CustomVertex(const D2D1_POINT_2F& pt) {
			x = pt.x;
			y = pt.y;
			z = 0;

			color = 0xff000000;
		}

		CustomVertex(const D2D1_POINT_2F& pt, uint32_t c) {
			x = pt.x;
			y = pt.y;
			z = 0;

			color = c;
		}

		CustomVertex(const CustomVertex& other) {
			x = other.x;
			y = other.y;
			z = other.z;

			color = other.color;
		}
	};	// 16 B

	static_assert(std::is_copy_constructible<CustomVertex>(), "CustomVertex must be trivially constructible");
	static_assert(std::is_standard_layout<CustomVertex>(), "CustomVertex must be a standard layout structure");

	struct VSConstants
	{
		Math::Matrix proj;
	};

private:
	std::vector<CustomVertex>	vertices;
	D3D11_VIEWPORT				viewport;	// first viewport from device

	ID3D11Device*				device;
	ID3D11DeviceContext*		context;

	ID3D11Buffer*				vertexbuffer;
	ID3D11Buffer*				vsconstants;

	ID3D11VertexShader*			vertexshader;
	ID3D11PixelShader*			pixelshader;
	ID3D11InputLayout*			inputlayout;
	ID3D11ShaderResourceView*	instancesrv;
	ID3D11RasterizerState*		rasterstate;

	ULONG						refcount;
	UINT						inctancecount;

	~D3D11InstancedRenderer();

public:
	D3D11InstancedRenderer(ID3D11Device* d3ddevice, ID3D11DeviceContext* d3dcontext);

	// IUnknown methods
	STDMETHOD(QueryInterface)(
		REFIID riid,
		void** ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef(void) override;
	ULONG STDMETHODCALLTYPE Release(void) override;

	// ID2D1TessellationSink methods
	STDMETHOD_(void, AddTriangles)(CONST D2D1_TRIANGLE *triangles, UINT32 trianglesCount) override;
	STDMETHOD(Close)() override;

	// specific
	bool SetGeometry(ID2D1PathGeometry* geometry, FLOAT strokewidth);
	bool SetInstanceData(UINT numinstances, std::function<void(UINT, Math::Matrix&)> callback);
	void Draw();
};

class D3D11LinearGradientBrush : public D3D11Brush
{
private:
	ID3D11ShaderResourceView*	gradientsrv;

public:
	D3D11LinearGradientBrush(
		ID3D11Device* d3ddevice, ID3D11DeviceContext* d3dcontext,
		const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES& props, const D2D1_GRADIENT_STOP* stops, UINT32 stopcount);

	~D3D11LinearGradientBrush();
};

#endif	// DIRECT3D11

#endif
