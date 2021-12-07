
#ifndef _DX11EXT_H_
#define _DX11EXT_H_

#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <gdiplus.h>

#include <iostream>
#include <string>
#include <cassert>

// --- Structures -------------------------------------------------------------

struct D3D11_ATTRIBUTE_RANGE
{
	UINT AttribId;
	UINT FaceStart;
	UINT FaceCount;
	UINT VertexStart;
	UINT VertexCount;
};

// --- Classes ----------------------------------------------------------------

class D3D11Mesh
{
	friend HRESULT DXLoadMeshFromQM(ID3D11Device*, LPCTSTR, DWORD, D3D11Mesh**);

private:
	ID3D11Device*				device;
	ID3D11Buffer*				vertexbuffer;
	ID3D11Buffer*				indexbuffer;
	ID3D11InputLayout*			inputlayout;
	D3D11_INPUT_ELEMENT_DESC*	vertexdecl;
	D3D11_ATTRIBUTE_RANGE*		subsettable;
	UINT						vertexstride;
	UINT						numsubsets;
	UINT						numvertexelements;
	DXGI_FORMAT					indexformat;

public:
	D3D11Mesh(ID3D11Device* d3ddevice);
	~D3D11Mesh();

	bool LinkToVertexShader(const void* signature, SIZE_T bytecodelength);

	void Draw(ID3D11DeviceContext* context);
	void DrawSubset(ID3D11DeviceContext* context, UINT subset);
};

class D3D11ScreenQuad
{
private:
	ID3D11VertexShader* vertexshader;
	ID3D11PixelShader* pixelshader;
	ID3D11InputLayout* inputlayout;

public:
	D3D11ScreenQuad(ID3D11Device* device, LPCWSTR effectfile);
	~D3D11ScreenQuad();

	void Draw(ID3D11DeviceContext* context);
};

// --- Functions --------------------------------------------------------------

//HRESULT DXCreateEffect(ID3D11Device* device, LPCTSTR file, ID3D10Effect** out);
HRESULT DXLoadMeshFromQM(ID3D11Device* device, LPCTSTR file, DWORD options, D3D11Mesh** mesh);
//HRESULT DXRenderTextEx(ID3D11Device* device, const std::string& str, uint32_t width, uint32_t height, const WCHAR* face, bool border, int style, float emsize, ID3D10ShaderResourceView** srv);

#endif
