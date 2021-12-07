
#include "d2dext.h"

#include <iostream>
#include <algorithm>

STDMETHODIMP OutlineTextRenderer::QueryInterface(REFIID riid, void** ppvObject)
{
	*ppvObject = NULL;

	if (__uuidof(IDWriteTextRenderer) == riid || __uuidof(IDWritePixelSnapping) == riid || __uuidof(IUnknown) == riid) {
		*ppvObject = this;
		return S_OK;
	}

	return E_FAIL;
}

ULONG STDMETHODCALLTYPE OutlineTextRenderer::AddRef(void)
{
	// ignore this shit
	return 1;
}

ULONG STDMETHODCALLTYPE OutlineTextRenderer::Release(void)
{
	// ignore this shit
	return 0;
}

OutlineTextRenderer::OutlineTextRenderer(ID2D1Factory* factory, ID2D1RenderTarget* target)
{
	d2dfactory = factory;
	rendertarget = target;

	rendertarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &blackbrush);
	rendertarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &whitebrush);

	D2D1_STROKE_STYLE_PROPERTIES strokeprops = D2D1::StrokeStyleProperties();

	strokeprops.lineJoin = D2D1_LINE_JOIN_ROUND;

	d2dfactory->CreateStrokeStyle(strokeprops, NULL, 0, &strokestyle);
}

OutlineTextRenderer::~OutlineTextRenderer()
{
	if (strokestyle)
		strokestyle->Release();

	if (blackbrush)
		blackbrush->Release();

	if (whitebrush)
		whitebrush->Release();
}

STDMETHODIMP OutlineTextRenderer::IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled)
{
	*isDisabled = FALSE;
	return S_OK;
}

STDMETHODIMP OutlineTextRenderer::GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform)
{
	rendertarget->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
	return S_OK;
}

STDMETHODIMP OutlineTextRenderer::GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip)
{
	*pixelsPerDip = 1.0f;
	return S_OK;
}

STDMETHODIMP OutlineTextRenderer::DrawGlyphRun(
	void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	DWRITE_MEASURING_MODE measuringMode,
	DWRITE_GLYPH_RUN const* glyphRun,
	DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
	IUnknown* clientDrawingEffect)
{
	ID2D1TransformedGeometry* transformedgeometry = nullptr;
	ID2D1PathGeometry* pathgeometry = nullptr;
	ID2D1GeometrySink* geometrysink = nullptr;

	d2dfactory->CreatePathGeometry(&pathgeometry);

	pathgeometry->Open(&geometrysink);
	{
		glyphRun->fontFace->GetGlyphRunOutline(
			glyphRun->fontEmSize,
			glyphRun->glyphIndices,
			glyphRun->glyphAdvances,
			glyphRun->glyphOffsets,
			glyphRun->glyphCount,
			glyphRun->isSideways,
			glyphRun->bidiLevel % 2,
			geometrysink);
	}
	geometrysink->Close();

	D2D1::Matrix3x2F transform = D2D1::Matrix3x2F(
		1.0f, 0.0f,
		0.0f, 1.0f,
		baselineOriginX, baselineOriginY);
	
	d2dfactory->CreateTransformedGeometry(pathgeometry, transform, &transformedgeometry);

	rendertarget->DrawGeometry(transformedgeometry, blackbrush, 3.0f, strokestyle);
	rendertarget->FillGeometry(transformedgeometry, whitebrush);
	
	transformedgeometry->Release();
	geometrysink->Release();
	pathgeometry->Release();

	return S_OK;
}

STDMETHODIMP OutlineTextRenderer::DrawUnderline(
	void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	DWRITE_UNDERLINE const* underline,
	IUnknown* clientDrawingEffect)
{
	return E_NOTIMPL;
}

STDMETHODIMP OutlineTextRenderer::DrawStrikethrough(
	void* clientDrawingContext,
	FLOAT baselineOriginX,
	FLOAT baselineOriginY,
	DWRITE_STRIKETHROUGH const* strikethrough,
	IUnknown* clientDrawingEffect)
{
	return E_NOTIMPL;
}

STDMETHODIMP OutlineTextRenderer::DrawInlineObject(
	void* clientDrawingContext,
	FLOAT originX,
	FLOAT originY,
	IDWriteInlineObject* inlineObject,
	BOOL isSideways,
	BOOL isRightToLeft,
	IUnknown* clientDrawingEffect)
{
	return E_NOTIMPL;
}

#ifdef DIRECT3D11
D3D11InstancedRenderer::D3D11InstancedRenderer(ID3D11Device* d3ddevice, ID3D11DeviceContext* d3dcontext)
{
	device			= d3ddevice;
	context			= d3dcontext;

	refcount		= 1;
	inctancecount	= 0;

	vertexbuffer	= nullptr;
	vsconstants		= nullptr;

	vertexshader	= nullptr;
	pixelshader		= nullptr;
	inputlayout		= nullptr;
	instancesrv		= nullptr;
	rasterstate		= nullptr;

	ID3DBlob*	code	= nullptr;
	ID3DBlob*	errors	= nullptr;
	HRESULT		hr		= S_OK;
	UINT		flags	= D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	hr = D3DCompileFromFile(
		L"../../Media/ShadersDX11/simplecolor.hlsl",
		NULL, NULL, "vs_simplecolor", "vs_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();
	}

	if (FAILED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &vertexshader))) {
		throw std::bad_alloc();
	}

	D3D11_INPUT_ELEMENT_DESC elemdesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (FAILED(device->CreateInputLayout(elemdesc, ARRAY_SIZE(elemdesc), code->GetBufferPointer(), code->GetBufferSize(), &inputlayout))) {
		throw std::bad_alloc();
	}

	code->Release();

	hr = D3DCompileFromFile(
		L"../../Media/ShadersDX11/simplecolor.hlsl",
		NULL, NULL, "ps_simplecolor", "ps_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();
	}

	if (FAILED(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &pixelshader))) {
		throw std::bad_alloc();
	}

	code->Release();

	vertices.reserve(12);

	// create uniform buffer
	D3D11_BUFFER_DESC desc;

	desc.ByteWidth				= sizeof(VSConstants);
	desc.BindFlags				= D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags			= 0;
	desc.Usage					= D3D11_USAGE_DEFAULT;
	desc.MiscFlags				= 0;
	desc.StructureByteStride	= 0;

	if (FAILED(device->CreateBuffer(&desc, NULL, &vsconstants))) {
		throw std::bad_alloc();
	}

	// create rasterizer state
	D3D11_RASTERIZER_DESC rasterdesc;

	rasterdesc.AntialiasedLineEnable	= FALSE;
	rasterdesc.CullMode					= D3D11_CULL_BACK;
	rasterdesc.DepthBias				= 0;
	rasterdesc.DepthBiasClamp			= 0;
	rasterdesc.DepthClipEnable			= TRUE;
	rasterdesc.FillMode					= D3D11_FILL_SOLID;
	rasterdesc.FrontCounterClockwise	= FALSE;
	rasterdesc.MultisampleEnable		= FALSE;
	rasterdesc.ScissorEnable			= FALSE;
	rasterdesc.SlopeScaledDepthBias		= 0;

	device->CreateRasterizerState(&rasterdesc, &rasterstate);

	// get viewport
	UINT numviewports = 0;
	D3D11_VIEWPORT* viewports = nullptr;

	context->RSGetViewports(&numviewports, NULL);

	viewports = new D3D11_VIEWPORT[numviewports];
	context->RSGetViewports(&numviewports, viewports);

	viewport = viewports[0];

	delete[] viewports;
}

D3D11InstancedRenderer::~D3D11InstancedRenderer()
{
	if (rasterstate)
		rasterstate->Release();

	if (inputlayout)
		inputlayout->Release();

	if (vertexshader)
		vertexshader->Release();

	if (pixelshader)
		pixelshader->Release();

	if (vsconstants)
		vsconstants->Release();

	if (instancesrv)
		instancesrv->Release();
	
	if (vertexbuffer)
		vertexbuffer->Release();
}

STDMETHODIMP D3D11InstancedRenderer::QueryInterface(REFIID riid, void** ppvObject)
{
	*ppvObject = NULL;

	if (__uuidof(ID2D1TessellationSink) == riid || __uuidof(IUnknown) == riid) {
		*ppvObject = this;
		return S_OK;
	}

	return E_FAIL;
}

ULONG STDMETHODCALLTYPE D3D11InstancedRenderer::AddRef(void)
{
	return InterlockedIncrement(&refcount);
}

ULONG STDMETHODCALLTYPE D3D11InstancedRenderer::Release(void)
{
	ULONG newcount = InterlockedDecrement(&refcount);

	if (newcount == 0) {
		delete this;
		return 0;
	}

	return newcount;
}

void STDMETHODCALLTYPE D3D11InstancedRenderer::AddTriangles(CONST D2D1_TRIANGLE *triangles, UINT32 trianglesCount)
{
	if (vertices.size() + trianglesCount * 3 > vertices.capacity()) {
		vertices.reserve(vertices.capacity() * 2);
	}

	for (UINT32 i = 0; i < trianglesCount; ++i) {
		const D2D1_TRIANGLE& tri = triangles[i];

		vertices.push_back(CustomVertex(tri.point1, 0xff00ff00));
		vertices.push_back(CustomVertex(tri.point2, 0xf00fff00));
		vertices.push_back(CustomVertex(tri.point3, 0xf00fff00));
	}
}

STDMETHODIMP D3D11InstancedRenderer::Close()
{
	if (vertexbuffer != nullptr)
		vertexbuffer->Release();

	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA initialdata;

	desc.ByteWidth					= (UINT)(vertices.size() * sizeof(CustomVertex));
	desc.BindFlags					= D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags				= 0;
	desc.Usage						= D3D11_USAGE_IMMUTABLE;
	desc.MiscFlags					= 0;
	desc.StructureByteStride		= 0;

	initialdata.pSysMem				= vertices.data();
	initialdata.SysMemPitch			= 0;
	initialdata.SysMemSlicePitch	= 0;

	HRESULT hr = device->CreateBuffer(&desc, &initialdata, &vertexbuffer);
	return SUCCEEDED(hr);
}

bool D3D11InstancedRenderer::SetGeometry(ID2D1PathGeometry* geometry, FLOAT strokewidth)
{
	ID2D1Factory* factory = nullptr;
	ID2D1PathGeometry* outlinegeometry = nullptr;
	ID2D1GeometrySink* outlinesink = nullptr;

	geometry->GetFactory(&factory);

	// tessellate outline
	factory->CreatePathGeometry(&outlinegeometry);
	outlinegeometry->Open(&outlinesink);
	
	geometry->Widen(strokewidth, NULL, D2D1::Matrix3x2F::Identity(), outlinesink);
	outlinesink->Close();

	outlinegeometry->Tessellate(D2D1::Matrix3x2F::Identity(), this);

	outlinesink->Release();
	outlinegeometry->Release();

	// tessellate inside
	//geometry->Tessellate(D2D1::Matrix3x2F::Identity(), d3d11renderer);
	
	if (FAILED(Close())) {
		std::cout << "Path geometry tessellation failed!\n";
		return false;
	}

	return true;
}

bool D3D11InstancedRenderer::SetInstanceData(UINT numinstances, std::function<void(UINT, Math::Matrix&)> callback)
{
	if (instancesrv != nullptr)
		instancesrv->Release();
	
	inctancecount = numinstances;

	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA initialdata;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	ID3D11Buffer* instancebuffer = nullptr;
	Math::Matrix* data = new Math::Matrix[numinstances];

	desc.BindFlags					= D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth					= numinstances * sizeof(Math::Matrix);
	desc.CPUAccessFlags				= 0;
	desc.MiscFlags					= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride		= sizeof(Math::Matrix);
	desc.Usage						= D3D11_USAGE_IMMUTABLE;

	initialdata.pSysMem				= data;
	initialdata.SysMemPitch			= 0;
	initialdata.SysMemSlicePitch	= 0;

	for (UINT i = 0; i < numinstances; ++i) {
		callback(i, data[i]);
	}

	if (FAILED(device->CreateBuffer(&desc, &initialdata, &instancebuffer)))
		return false;

	delete[] data;

	srvdesc.Format = DXGI_FORMAT_UNKNOWN;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	srvdesc.Buffer.FirstElement = 0;
	srvdesc.Buffer.NumElements = numinstances;

	if (FAILED(device->CreateShaderResourceView(instancebuffer, &srvdesc, &instancesrv))) {
		instancebuffer->Release();
		return false;
	}

	instancebuffer->Release();
	return true;
}

void D3D11InstancedRenderer::Draw()
{
	UINT offset = 0;
	UINT stride = sizeof(CustomVertex);

	// update uniforms
	VSConstants vsuniforms;

	Math::MatrixOrthoOffCenterLH(
		vsuniforms.proj,
		viewport.TopLeftX,
		viewport.TopLeftX + viewport.Width,
		viewport.TopLeftY + viewport.Height,
		viewport.TopLeftY,
		-1, 1);

	Math::MatrixTranspose(vsuniforms.proj, vsuniforms.proj);

	context->UpdateSubresource(vsconstants, 0, NULL, &vsuniforms, sizeof(VSConstants), sizeof(VSConstants));
	
	// render
	context->IASetInputLayout(inputlayout);
	context->IASetVertexBuffers(0, 1, &vertexbuffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(vertexshader, NULL, 0);
	context->VSSetConstantBuffers(0, 1, &vsconstants);
	context->VSSetShaderResources(0, 1, &instancesrv);

	context->PSSetShader(pixelshader, NULL, 0);
	context->RSSetState(rasterstate);

	context->DrawInstanced((UINT)vertices.size(), inctancecount, 0, 0);
}

D3D11LinearGradientBrush::D3D11LinearGradientBrush(
	ID3D11Device* d3ddevice, ID3D11DeviceContext* d3dcontext,
	const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES& props, const D2D1_GRADIENT_STOP* stops, UINT32 stopcount)
{
	gradientsrv = nullptr;

	// TODO: std::partial_sort_copy

	D3D11_TEXTURE1D_DESC desc;
	D3D11_SUBRESOURCE_DATA initialdata;
	ID3D11Texture1D* texture = nullptr;

	desc.ArraySize		= 1;
	desc.BindFlags		= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags	= 0;
	desc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.MipLevels		= 1;
	desc.MiscFlags		= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	desc.Usage			= D3D11_USAGE_DEFAULT;
	desc.Width			= 1024;

	UINT32* colordata = new UINT32[1024];
	Math::Color clow, chigh, cmid;

	for (int i = 0; i < 1024; ++i) {
		float pos = i / 1023.0f;

		const D2D1_GRADIENT_STOP& first = stops[0];
		clow = Math::Color(first.color.r, first.color.g, first.color.b, first.color.a);
		
		if (pos < stops[0].position) {
			colordata[i] = (uint32_t)clow;
		} else {
			for (UINT32 j = 1; j < stopcount; ++j) {
				const D2D1_GRADIENT_STOP& prev = stops[j - 1];
				const D2D1_GRADIENT_STOP& next = stops[j];

				chigh = Math::Color(next.color.r, next.color.g, next.color.b, next.color.a);
				cmid = chigh;

				if (pos >= prev.position && pos <= next.position) {
					if (prev.position == next.position)
						cmid = clow;
					else
						cmid = Math::Color::Lerp(clow, chigh, (pos - prev.position) / (next.position - prev.position));

					break;
				}

				clow = chigh;
			}

			colordata[i] = (uint32_t)cmid;
		}
	}

	initialdata.pSysMem				= colordata;
	initialdata.SysMemPitch			= desc.Width * 4;
	initialdata.SysMemSlicePitch	= initialdata.SysMemPitch;

	if (FAILED(d3ddevice->CreateTexture1D(&desc, &initialdata, &texture))) {
		throw std::bad_alloc();
	}

	delete[] colordata;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;

	srvdesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE1D;
	srvdesc.Format						= DXGI_FORMAT_B8G8R8A8_UNORM;
	srvdesc.Texture1D.MipLevels			= -1;
	srvdesc.Texture1D.MostDetailedMip	= 0;

	if (FAILED(d3ddevice->CreateShaderResourceView(texture, &srvdesc, &gradientsrv))) {
		throw std::bad_alloc();
	}

	d3dcontext->GenerateMips(gradientsrv);
	texture->Release();
}

D3D11LinearGradientBrush::~D3D11LinearGradientBrush()
{
	if (gradientsrv)
		gradientsrv->Release();
}

#endif
