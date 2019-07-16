
#ifndef _DX10EXT_H_
#define _DX10EXT_H_

#include <d3d10.h>
#include <d3dx10.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <cassert>

// --- Functions --------------------------------------------------------------

HRESULT DXCreateEffect(ID3D10Device* device, LPCTSTR file, ID3D10Effect** out);
HRESULT DXLoadMeshFromQM(ID3D10Device* device, LPCTSTR file, DWORD options, ID3DX10Mesh** mesh);
HRESULT DXRenderTextEx(ID3D10Device* device, const std::string& str, uint32_t width, uint32_t height, const WCHAR* face, bool border, int style, float emsize, ID3D10ShaderResourceView** srv);

#endif
