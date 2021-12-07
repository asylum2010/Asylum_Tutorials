
#include "dx11ext.h"
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

// --- Classes impl -----------------------------------------------------------

D3D11Mesh::D3D11Mesh(ID3D11Device* d3ddevice)
{
	device				= d3ddevice;
	vertexbuffer		= NULL;
	indexbuffer			= NULL;
	inputlayout			= NULL;
	vertexdecl			= NULL;
	subsettable			= NULL;

	vertexstride		= 0;
	numsubsets			= 0;
	numvertexelements	= 0;
	indexformat			= DXGI_FORMAT_UNKNOWN;
}

D3D11Mesh::~D3D11Mesh()
{
	if (vertexbuffer)
		vertexbuffer->Release();

	if (indexbuffer)
		indexbuffer->Release();

	if (inputlayout)
		inputlayout->Release();

	delete[] vertexdecl;
	delete[] subsettable;
}

bool D3D11Mesh::LinkToVertexShader(const void* signature, SIZE_T bytecodelength)
{
	if (inputlayout) {
		inputlayout->Release();
		inputlayout = NULL;
	}

	HRESULT hr = device->CreateInputLayout(vertexdecl, numvertexelements, signature, bytecodelength, &inputlayout);
	return SUCCEEDED(hr);
}

void D3D11Mesh::Draw(ID3D11DeviceContext* context)
{
	for (UINT i = 0; i < numsubsets; ++i)
		DrawSubset(context, i);
}

void D3D11Mesh::DrawSubset(ID3D11DeviceContext* context, UINT subset)
{
	const D3D11_ATTRIBUTE_RANGE& attrange = subsettable[subset];
	UINT voffset = 0; //attrange.VertexStart * vertexstride;

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(inputlayout);
	context->IASetVertexBuffers(0, 1, &vertexbuffer, &vertexstride, &voffset);
	context->IASetIndexBuffer(indexbuffer, indexformat, 0);

	context->DrawIndexed(attrange.FaceCount * 3, attrange.FaceStart * 3, attrange.VertexStart);
}

// --- Functions impl ---------------------------------------------------------

HRESULT DXLoadMeshFromQM(ID3D11Device* device, LPCTSTR file, DWORD options, D3D11Mesh** mesh)
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

	Math::Color	color;
	FILE*		infile = NULL;
	uint32_t	unused;
	uint32_t	version;
	uint32_t	numindices;
	uint32_t	numvertices;
	uint32_t	vstride;
	uint32_t	istride;
	uint32_t	numsubsets;
	uint32_t	numelems;
	uint16_t	tmp16;
	uint8_t		tmp8;
	void*		data;
	char		buff[256];

	(*mesh) = nullptr;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb")
#endif

	if (!infile)
		return E_FAIL;

	D3D11Mesh* d3dmesh = new D3D11Mesh(device);

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	d3dmesh->subsettable = new D3D11_ATTRIBUTE_RANGE[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	d3dmesh->vertexdecl = new D3D11_INPUT_ELEMENT_DESC[numelems];

	vstride = 0;

	for (uint32_t i = 0; i < numelems; ++i) {
		fread(&tmp16, 2, 1, infile);
		d3dmesh->vertexdecl[i].InputSlot = tmp16;
		d3dmesh->vertexdecl[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		fread(&tmp8, 1, 1, infile);
		d3dmesh->vertexdecl[i].SemanticName = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		d3dmesh->vertexdecl[i].Format = types[tmp8];

		unused = elemsizes[tmp8] * elemstrides[tmp8];

		fread(&tmp8, 1, 1, infile);
		d3dmesh->vertexdecl[i].SemanticIndex = tmp8;

		d3dmesh->vertexdecl[i].AlignedByteOffset = vstride;
		d3dmesh->vertexdecl[i].InstanceDataStepRate = 0;

		vstride += unused;
	}

	d3dmesh->vertexstride = vstride;

	// create buffers
	D3D11_BUFFER_DESC buffdesc;
	D3D11_SUBRESOURCE_DATA initialdata;

	buffdesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	buffdesc.ByteWidth				= numvertices * vstride;
	buffdesc.CPUAccessFlags			= 0;
	buffdesc.MiscFlags				= 0;
	buffdesc.StructureByteStride	= 0;
	buffdesc.Usage					= D3D11_USAGE_DEFAULT;

	data = malloc(buffdesc.ByteWidth);
	fread(data, vstride, numvertices, infile);

	initialdata.pSysMem				= data;
	initialdata.SysMemPitch			= 0;
	initialdata.SysMemSlicePitch	= 0;

	if (FAILED(device->CreateBuffer(&buffdesc, &initialdata, &d3dmesh->vertexbuffer))) {
		std::cout << "DXLoadMeshFromQM(): Could not create vertex buffer\n";
		return E_FAIL;
	}

	free(data);

	buffdesc.BindFlags				= D3D11_BIND_INDEX_BUFFER;
	buffdesc.ByteWidth				= numindices * istride;

	data = malloc(buffdesc.ByteWidth);
	fread(data, istride, numindices, infile);

	initialdata.pSysMem				= data;

	if (FAILED(device->CreateBuffer(&buffdesc, &initialdata, &d3dmesh->indexbuffer))) {
		std::cout << "DXLoadMeshFromQM(): Could not create index buffer\n";
		return E_FAIL;
	}

	free(data);
	
	d3dmesh->numsubsets = numsubsets;
	d3dmesh->numvertexelements = numelems;

	if (istride == 4)
		d3dmesh->indexformat = DXGI_FORMAT_R32_UINT;
	else
		d3dmesh->indexformat = DXGI_FORMAT_R16_UINT;

	if (version >= 1) {
		// skip LOD tree
		fread(&unused, 4, 1, infile);
		fseek(infile, 8 * unused, SEEK_CUR);
	}

	// subset table
	for (uint32_t i = 0; i < numsubsets; ++i) {
		D3D11_ATTRIBUTE_RANGE& subset = d3dmesh->subsettable[i];

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
			fread(&color, sizeof(Math::Color), 1, infile);
			//mat.MatD3D.Ambient = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(Math::Color), 1, infile);
			//mat.MatD3D.Diffuse = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(Math::Color), 1, infile);
			//mat.MatD3D.Specular = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(Math::Color), 1, infile);
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

	(*mesh) = d3dmesh;

	return S_OK;
}

D3D11ScreenQuad::D3D11ScreenQuad(ID3D11Device* device, LPCWSTR effectfile)
{
	vertexshader	= nullptr;
	pixelshader		= nullptr;
	inputlayout		= nullptr;

	ID3DBlob*	code	= nullptr;
	ID3DBlob*	errors	= nullptr;
	UINT		flags	= D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		effectfile,
		NULL, NULL, "vs_main", "vs_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();

		throw std::bad_alloc();
	}

	if (FAILED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &vertexshader))) {
		throw std::bad_alloc();
	}

	D3D11_INPUT_ELEMENT_DESC elems[] = {
		{ "SV_VertexID", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (FAILED(device->CreateInputLayout(elems, 1, code->GetBufferPointer(), code->GetBufferSize(), &inputlayout))) {
		throw std::bad_alloc();
	}

	code->Release();

	hr = D3DCompileFromFile(
		effectfile,
		NULL, NULL, "ps_main", "ps_5_0",
		flags, 0, &code, &errors);

	if (errors != nullptr) {
		std::cout << (char*)errors->GetBufferPointer() << std::endl;
		errors->Release();

		throw std::bad_alloc();
	}

	if (FAILED(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &pixelshader))) {
		throw std::bad_alloc();
	}

	code->Release();
}

D3D11ScreenQuad::~D3D11ScreenQuad()
{
	if (inputlayout)
		inputlayout->Release();

	if (vertexshader)
		vertexshader->Release();

	if (pixelshader)
		pixelshader->Release();
}

void D3D11ScreenQuad::Draw(ID3D11DeviceContext* context)
{
	ID3D11Buffer* buffers[] = { NULL };
	UINT unused = 0;

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->IASetInputLayout(inputlayout);
	context->IASetVertexBuffers(0, 1, buffers, &unused, &unused);

	context->VSSetShader(vertexshader, NULL, 0);
	context->PSSetShader(pixelshader, NULL, 0);

	context->Draw(4, 0);
}
