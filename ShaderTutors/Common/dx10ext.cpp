
#include "dx10ext.h"
#include "3Dmath.h"

static void ReadString(FILE* f, char* buff)
{
	size_t ind = 0;
	char ch = fgetc(f);

	while (ch != '\n') {
		buff[ind] = ch;
		ch = fgetc(f);

		++ind;
	}

	buff[ind] = '\0';
}

// --- Functions impl ---------------------------------------------------------

HRESULT DXCreateEffect(ID3D10Device* device, LPCTSTR file, ID3D10Effect** out)
{
	ID3D10Blob*	errors = nullptr;
	UINT hlslflags = D3D10_SHADER_ENABLE_STRICTNESS;

#ifdef _DEBUG
	hlslflags |= D3D10_SHADER_DEBUG;
#endif

	HRESULT hr = D3DX10CreateEffectFromFile(file, 0, 0, "fx_4_0", hlslflags, 0, device, 0, 0, out, &errors, 0);

	if (errors != nullptr) {
		char* str = (char*)errors->GetBufferPointer();
		std::cout << str << "\n";

		errors->Release();
	}

	return hr;
}

HRESULT DXLoadMeshFromQM(ID3D10Device* device, LPCTSTR file, DWORD options, ID3DX10Mesh** mesh)
{
	static const char* usages[] = {
		"POSITION",
		"POSITIONT",
		"COLOR",
		"BLENDWEIGHT",
		"BLENDINDICES",
		"NORMAL",
		"TEXCOORD",
		"TANGENT",
		"BINORMAL",
		"PSIZE",
		"TESSFACTOR"
	};

	static const DXGI_FORMAT types[] = {
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};

	static const unsigned short elemsizes[6] = {
		1, 2, 3, 4, 4, 4
	};

	static const unsigned short elemstrides[6] = {
		4, 4, 4, 4, 1, 1
	};

	D3D10_INPUT_ELEMENT_DESC*	decl;
	D3DX10_ATTRIBUTE_RANGE*		table;
	ID3DX10MeshBuffer*			vertexbuffer;
	ID3DX10MeshBuffer*			indexbuffer;
	ID3DX10MeshBuffer*			attribbuffer;
	SIZE_T						datasize;

	D3DXCOLOR					color;
	HRESULT						hr;
	FILE*						infile = nullptr;
	uint32_t					unused;
	uint32_t					version;
	uint32_t					numindices;
	uint32_t					numvertices;
	uint32_t					vstride;
	uint32_t					istride;
	uint32_t					numsubsets;
	uint32_t					numelems;
	uint16_t					tmp16;
	uint8_t						tmp8;
	void*						data;
	char						buff[256];

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb")
#endif

	if (!infile)
		return E_FAIL;

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	table = new D3DX10_ATTRIBUTE_RANGE[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new D3D10_INPUT_ELEMENT_DESC[numelems];

	vstride = 0;

	for (uint32_t i = 0; i < numelems; ++i) {
		fread(&tmp16, 2, 1, infile);
		decl[i].InputSlot = tmp16;
		decl[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;

		fread(&tmp8, 1, 1, infile);
		decl[i].SemanticName = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].Format = types[tmp8];

		unused = elemsizes[tmp8] * elemstrides[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].SemanticIndex = tmp8;

		decl[i].AlignedByteOffset = vstride;
		decl[i].InstanceDataStepRate = 0;

		vstride += unused;
	}

	// create mesh
	if (istride == 4)
		options |= D3DX10_MESH_32_BIT;

	hr = D3DX10CreateMesh(device, decl, numelems, "POSITION", numvertices, numindices / 3, options, mesh);

	if (FAILED(hr))
		goto _fail;

	(*mesh)->GetVertexBuffer(0, &vertexbuffer);
	(*mesh)->GetIndexBuffer(&indexbuffer);

	vertexbuffer->Map(&data, &datasize);
	{
		fread(data, vstride, numvertices, infile);
	}
	vertexbuffer->Unmap();
	vertexbuffer->Release();

	indexbuffer->Map(&data, &datasize);
	{
		fread(data, istride, numindices, infile);
	}
	indexbuffer->Unmap();
	indexbuffer->Release();

	if (version >= 1) {
		// skip LOD tree
		fread(&unused, 4, 1, infile);
		fseek(infile, 8 * unused, SEEK_CUR);
	}

	for (uint32_t i = 0; i < numsubsets; ++i) {
		D3DX10_ATTRIBUTE_RANGE& subset = table[i];

		subset.AttribId = i;

		fread(&subset.FaceStart, 4, 1, infile);
		fread(&subset.VertexStart, 4, 1, infile);
		fread(&subset.VertexCount, 4, 1, infile);
		fread(&subset.FaceCount, 4, 1, infile);

		subset.FaceCount /= 3;
		subset.FaceStart /= 3;

		fseek(infile, 6 * sizeof(float), SEEK_CUR);

		ReadString(infile, buff);
		ReadString(infile, buff);

		bool hasmaterial = (buff[1] != ',');

		if (hasmaterial) {
			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Ambient = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Diffuse = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Specular = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Emissive = (D3DCOLORVALUE&)color;

			fread(&color.r, sizeof(float), 1, infile);	// mat.MatD3D.Power
			fread(&color.a, sizeof(float), 1, infile);	// mat.MatD3D.Diffuse.a

			fread(&unused, 4, 1, infile);
			ReadString(infile, buff);

			//if (buff[1] != ',') {
			//	unused = strlen(buff);

			//	mat.pTextureFilename = new char[unused + 1];
			//	memcpy(mat.pTextureFilename, buff, unused);
			//	mat.pTextureFilename[unused] = 0;
			//}

			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
		} else {
			//color = D3DXCOLOR(1, 1, 1, 1);

			//memcpy(&mat.MatD3D.Ambient, &color, 4 * sizeof(float));
			//memcpy(&mat.MatD3D.Diffuse, &color, 4 * sizeof(float));
			//memcpy(&mat.MatD3D.Specular, &color, 4 * sizeof(float));

			//color = D3DXCOLOR(0, 0, 0, 1);
			//memcpy(&mat.MatD3D.Emissive, &color, 4 * sizeof(float));

			//mat.MatD3D.Power = 80.0f;
		}

		ReadString(infile, buff);

		//if (buff[1] != ','  && mat.pTextureFilename == nullptr) {
		//	unused = strlen(buff);

		//	mat.pTextureFilename = new char[unused + 1];
		//	memcpy(mat.pTextureFilename, buff, unused);
		//	mat.pTextureFilename[unused] = 0;
		//}

		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
	}

	// attribute buffer
	(*mesh)->SetAttributeTable(table, numsubsets);
	(*mesh)->GetAttributeBuffer(&attribbuffer);

	attribbuffer->Map(&data, &datasize);

	for (uint32_t i = 0; i < numsubsets; ++i) {
		const D3DX10_ATTRIBUTE_RANGE& subset = table[i];

		for (DWORD j = 0; j < subset.FaceCount; ++j)
			*((DWORD*)data + subset.FaceStart + j) = subset.AttribId;
	}

	attribbuffer->Unmap();
	attribbuffer->Release();

	(*mesh)->CommitToDevice();

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return hr;
}

HRESULT DXRenderTextEx(ID3D10Device* device, const std::string& str, uint32_t width, uint32_t height, const WCHAR* face, bool border, int style, float emsize, ID3D10ShaderResourceView** srv)
{
	if (srv == nullptr)
		return E_FAIL;

	ID3D10Texture2D*		target = nullptr;
	D3D10_TEXTURE2D_DESC	texdesc;
	HRESULT					hr = S_OK;

	if (*srv != nullptr)
		(*srv)->GetResource((ID3D10Resource**)&target);

	texdesc.Width				= width;
	texdesc.Height				= height;
	texdesc.MipLevels			= 1;
	texdesc.ArraySize			= 1;
	texdesc.SampleDesc.Count	= 1;
	texdesc.SampleDesc.Quality	= 0;
	texdesc.Usage				= D3D10_USAGE_DYNAMIC;
	texdesc.CPUAccessFlags		= D3D10_CPU_ACCESS_WRITE;
	texdesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	texdesc.BindFlags			= D3D10_BIND_SHADER_RESOURCE;
	texdesc.MiscFlags			= 0;

	if (target == nullptr) {
		if (FAILED(hr = device->CreateTexture2D(&texdesc, NULL, &target)))
			return hr;
	}

	Gdiplus::Color			outline(0xff000000);
	Gdiplus::Color			fill(0xffffffff);

	Gdiplus::Bitmap*		bitmap;
	Gdiplus::Graphics*		graphics;
	Gdiplus::GraphicsPath	path;
	Gdiplus::FontFamily		family(face);
	Gdiplus::StringFormat	format;
	Gdiplus::Pen			pen(outline, 3);
	Gdiplus::SolidBrush		brush(border ? fill : outline);
	std::wstring			wstr(str.begin(), str.end());

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), (INT)wstr.length(), &family, style, emsize, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	if (border)
		graphics->DrawPath(&pen, &path);

	graphics->FillPath(&brush, &path);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;
	D3D10_MAPPED_TEXTURE2D d3drc;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));

	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	hr = target->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &d3drc);

	memcpy(d3drc.pData, data.Scan0, width * height * 4);

	target->Unmap(0);
	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;

	// create shader resource view if needed
	D3D10_SHADER_RESOURCE_VIEW_DESC srvdesc;

	srvdesc.Format = texdesc.Format;
	srvdesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MipLevels = 1;
	srvdesc.Texture2D.MostDetailedMip = 0;

	if (*srv == nullptr) {
		device->CreateShaderResourceView(target, &srvdesc, srv);
	}

	target->Release();
	return hr;
}
