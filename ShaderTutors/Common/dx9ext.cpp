
#include "dx9ext.h"
#include "3Dmath.h"
#include "geometryutils.h"

#define MAX_BONE_MATRICES	26

float DXScreenQuadVertices[24] = {
	-1, -1, 0, 1,	0, 1,
	-1, 1, 0, 1,	0, 0,
	1, -1, 0, 1,	1, 1,
	1, 1, 0, 1,		1, 0
};

float DXScreenQuadVerticesFFP[24] = {
	// NOTE: viewport must be added
	-0.5f, -0.5f, 0, 1,		0, 1,
	-0.5f, -0.5f, 0, 1,		0, 0,
	-0.5f, -0.5f, 0, 1,		1, 1
	-0.5f, -0.5f, 0, 1,		1, 0,
};

D3DXVECTOR3 DXCubeForward[6] = {
	D3DXVECTOR3(1, 0, 0),	// +X
	D3DXVECTOR3(-1, 0, 0),	// -X
	D3DXVECTOR3(0, 1, 0),	// +Y
	D3DXVECTOR3(0, -1, 0),	// -Y
	D3DXVECTOR3(0, 0, 1),	// +Z
	D3DXVECTOR3(0, 0, -1),	// -Z
};

D3DXVECTOR3 DXCubeUp[6] = {
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 0, -1),
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
};

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

HRESULT DXCreateEffect(LPDIRECT3DDEVICE9 device, LPCTSTR file, LPD3DXEFFECT* out)
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	if (FAILED(hr = D3DXCreateEffectFromFile(device, file, NULL, NULL, D3DXSHADER_DEBUG, NULL, out, &errors))) {
		if (errors) {
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";
		}
	}

	if (errors)
		errors->Release();

	return hr;
}

HRESULT DXLoadMeshFromQM(LPCTSTR file, DWORD options, LPDIRECT3DDEVICE9 d3ddevice, D3DXMATERIAL** materials, DWORD* nummaterials, LPD3DXMESH* mesh)
{
	static const uint8_t usages[] = {
		D3DDECLUSAGE_POSITION,
		D3DDECLUSAGE_POSITIONT,
		D3DDECLUSAGE_COLOR,
		D3DDECLUSAGE_BLENDWEIGHT,
		D3DDECLUSAGE_BLENDINDICES,
		D3DDECLUSAGE_NORMAL,
		D3DDECLUSAGE_TEXCOORD,
		D3DDECLUSAGE_TANGENT,
		D3DDECLUSAGE_BINORMAL,
		D3DDECLUSAGE_PSIZE,
		D3DDECLUSAGE_TESSFACTOR
	};

	static const uint16_t elemsizes[6] = {
		1, 2, 3, 4, 4, 4
	};

	static const uint16_t elemstrides[6] = {
		4, 4, 4, 4, 1, 1
	};

	D3DVERTEXELEMENT9*	decl;
	D3DXATTRIBUTERANGE*	table;
	D3DXCOLOR			color;
	HRESULT				hr;
	FILE*				infile = 0;
	uint32_t			unused;
	uint32_t			version;
	uint32_t			numindices;
	uint32_t			numvertices;
	uint32_t			vstride;
	uint32_t			istride;
	uint32_t			numsubsets;
	uint32_t			numelems;
	uint16_t			tmp16;
	uint8_t				tmp8;
	void*				data;
	char				buff[256];

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

	table = new D3DXATTRIBUTERANGE[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new D3DVERTEXELEMENT9[numelems + 1];

	vstride = 0;

	for (uint32_t i = 0; i < numelems; ++i) {
		fread(&tmp16, 2, 1, infile);
		decl[i].Stream = tmp16;

		fread(&tmp8, 1, 1, infile);
		decl[i].Usage = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].Type = tmp8;

		fread(&tmp8, 1, 1, infile);
		decl[i].UsageIndex = tmp8;

		decl[i].Method = D3DDECLMETHOD_DEFAULT;
		decl[i].Offset = vstride;

		vstride += elemsizes[decl[i].Type] * elemstrides[decl[i].Type];
	}

	decl[numelems].Stream		= 0xff;
	decl[numelems].Offset		= 0;
	decl[numelems].Type			= D3DDECLTYPE_UNUSED;
	decl[numelems].Method		= 0;
	decl[numelems].Usage		= 0;
	decl[numelems].UsageIndex	= 0;

	// create mesh
	if (istride == 4)
		options |= D3DXMESH_32BIT;

	hr = D3DXCreateMesh(numindices / 3, numvertices, options, decl, d3ddevice, mesh);

	if (FAILED(hr))
		goto _fail;

	(*mesh)->LockVertexBuffer(0, &data);
	fread(data, vstride, numvertices, infile);
	(*mesh)->UnlockVertexBuffer();

	(*mesh)->LockIndexBuffer(0, &data);
	fread(data, istride, numindices, infile);
	(*mesh)->UnlockIndexBuffer();

	if (version >= 1) {
		// skip LOD tree
		fread(&unused, 4, 1, infile);

		if (unused > 0)
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	// attribute table
	(*materials) = new D3DXMATERIAL[numsubsets];

	for (uint32_t i = 0; i < numsubsets; ++i) {
		D3DXATTRIBUTERANGE& subset = table[i];
		D3DXMATERIAL& mat = (*materials)[i];

		mat.pTextureFilename = nullptr;
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
			mat.MatD3D.Ambient = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Diffuse = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Specular = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Emissive = (D3DCOLORVALUE&)color;

			if (version >= 2)
				fseek(infile, 16, SEEK_CUR);	// uv scale

			fread(&mat.MatD3D.Power, sizeof(float), 1, infile);
			fread(&mat.MatD3D.Diffuse.a, sizeof(float), 1, infile);

			fread(&unused, 4, 1, infile);		// blend mode
			ReadString(infile, buff);

			if (buff[1] != ',') {
				unused = (uint32_t)strlen(buff);

				mat.pTextureFilename = new char[unused + 1];
				memcpy(mat.pTextureFilename, buff, unused);
				mat.pTextureFilename[unused] = 0;
			}

			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
		} else {
			color = D3DXCOLOR(1, 1, 1, 1);

			memcpy(&mat.MatD3D.Ambient, &color, 4 * sizeof(float));
			memcpy(&mat.MatD3D.Diffuse, &color, 4 * sizeof(float));
			memcpy(&mat.MatD3D.Specular, &color, 4 * sizeof(float));

			color = D3DXCOLOR(0, 0, 0, 1);
			memcpy(&mat.MatD3D.Emissive, &color, 4 * sizeof(float));

			mat.MatD3D.Power = 80.0f;
		}

		ReadString(infile, buff);

		if (buff[1] != ',' && mat.pTextureFilename == 0) {
			unused = (uint32_t)strlen(buff);

			mat.pTextureFilename = new char[unused + 1];
			memcpy(mat.pTextureFilename, buff, unused);
			mat.pTextureFilename[unused] = 0;
		}

		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
	}

	// attribute buffer
	(*mesh)->LockAttributeBuffer(0, (DWORD**)&data);

	for (uint32_t i = 0; i < numsubsets; ++i) {
		const D3DXATTRIBUTERANGE& subset = table[i];

		for (DWORD j = 0; j < subset.FaceCount; ++j)
			*((DWORD*)data + subset.FaceStart + j) = subset.AttribId;
	}

	(*mesh)->UnlockAttributeBuffer();
	(*mesh)->SetAttributeTable(table, numsubsets);

	*nummaterials = numsubsets;

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return hr;
}

HRESULT DXCreateChecker(LPDIRECT3DDEVICE9 device, UINT xseg, UINT yseg, DWORD color1, DWORD color2, LPDIRECT3DTEXTURE9* texture)
{
	UINT width = xseg * 50;
	UINT height = xseg * 50;

	D3DLOCKED_RECT rect;
	HRESULT hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);

	if (FAILED(hr))
		return hr;

	(*texture)->LockRect(0, &rect, NULL, 0);

	UINT xstep = width / xseg;
	UINT ystep = height / yseg;
	UINT a, b;

	xseg = xstep * xseg;
	yseg = ystep * yseg;

	for (UINT i = 0; i < height; ++i) {
		for (UINT j = 0; j < width; ++j) {
			DWORD* ptr = ((DWORD*)rect.pBits + i * width + j);

			a = i / ystep;
			b = j / xstep;

			if( (a + b) % 2 )
				*ptr = color1;
			else
				*ptr = color2;
		}
	}

	(*texture)->UnlockRect(0);
	return S_OK;
}

HRESULT DXCreatePlane(LPDIRECT3DDEVICE9 device, float width, float depth, LPD3DXMESH* mesh)
{
	D3DVERTEXELEMENT9 vertexlayout[] = {
		{ 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	GeometryUtils::CommonVertex* vdata = nullptr;
	uint32_t* idata = nullptr;
	DWORD* adata = nullptr;

	if (FAILED(D3DXCreateMesh(2, 4, D3DXMESH_MANAGED|D3DXMESH_32BIT, vertexlayout, device, mesh)))
		return E_FAIL;

	(*mesh)->LockVertexBuffer(0, (LPVOID*)&vdata);
	(*mesh)->LockIndexBuffer(0, (LPVOID*)&idata);
	{
		GeometryUtils::CreatePlane(vdata, idata, width, depth);
	}
	(*mesh)->UnlockIndexBuffer();
	(*mesh)->UnlockVertexBuffer();

	LPD3DXATTRIBUTERANGE subsettable = new D3DXATTRIBUTERANGE[1];

	subsettable[0].AttribId		= 0;
	subsettable[0].FaceCount	= 2;
	subsettable[0].FaceStart	= 0;
	subsettable[0].VertexCount	= 4;
	subsettable[0].VertexStart	= 0;

	(*mesh)->LockAttributeBuffer(0, &adata);
	{
		memset(adata, 0, 2 * sizeof(DWORD));
	}
	(*mesh)->UnlockAttributeBuffer();

	(*mesh)->SetAttributeTable(subsettable, 1);
	delete[] subsettable;

	return S_OK;
}

HRESULT DXGenTangentFrame(LPDIRECT3DDEVICE9 device, LPD3DXMESH mesh, LPD3DXMESH* newmesh)
{
	HRESULT hr;
	LPD3DXMESH clonedmesh = NULL;

	D3DVERTEXELEMENT9 decl[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
		{ 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
		D3DDECL_END()
	};

	// it is safer to clone
	hr = mesh->CloneMesh(D3DXMESH_MANAGED, decl, device, &clonedmesh);

	if (FAILED(hr))
		return hr;

	if (*newmesh == mesh) {
		mesh->Release();
		*newmesh = NULL;
	}

	hr = D3DXComputeTangentFrameEx(clonedmesh, D3DDECLUSAGE_TEXCOORD, 0,
		D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_NORMAL, 0,
		0, NULL, 0.01f, 0.25f, 0.01f, newmesh, NULL);

	clonedmesh->Release();
	return hr;
}

HRESULT DXRenderTextEx(LPDIRECT3DDEVICE9 device, const std::string& str, uint32_t width, uint32_t height, const WCHAR* face, bool border, int style, float emsize, LPDIRECT3DTEXTURE9* texture)
{
	if (texture == nullptr)
		return E_FAIL;

	LPDIRECT3DTEXTURE9 target = (*texture);
	HRESULT hr = D3D_OK;

	if (target == nullptr) {
		if (FAILED(hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &target, NULL)))
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
	D3DLOCKED_RECT d3drc;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));

	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	target->LockRect(0, &d3drc, 0, 0);

	memcpy(d3drc.pBits, data.Scan0, width * height * 4);

	target->UnlockRect(0);
	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;

	if ((*texture) == nullptr)
		(*texture) = target;

	return hr;
}

// --- DXAnimatedMesh impl ----------------------------------------------------

DXAnimatedMesh::DXAnimatedMesh()
{
	method			= SkinningMethodSoftware;
	effect			= nullptr;

	device			= nullptr;
	root			= nullptr;
	controller		= nullptr;
	bonetransforms	= nullptr;

	numanimations	= 0;
	maxnumbones		= 0;
	currenttime		= 0;
}

DXAnimatedMesh::~DXAnimatedMesh()
{
	if (root != nullptr) {
		D3DXFrameDestroy(root, this);
		root = nullptr;
	}

	if (controller != nullptr) {
		controller->Release();
		controller = nullptr;
	}

	if (bonetransforms != nullptr) {
		delete[] bonetransforms;
		bonetransforms = nullptr;
	}

	if (effect != nullptr)
		effect->Release();
}

HRESULT DXAnimatedMesh::CreateFrame(LPCSTR Name, LPD3DXFRAME* retNewFrame)
{
	D3DXFRAME_EXTENDED* newframe = new D3DXFRAME_EXTENDED;
	memset(newframe, 0, sizeof(D3DXFRAME_EXTENDED));
	
	*retNewFrame = 0;

	D3DXMatrixIdentity(&newframe->TransformationMatrix);
	D3DXMatrixIdentity(&newframe->exCombinedTransformationMatrix);

	if (Name) {
		size_t len = strlen(Name);

		if (len > 0) {
			newframe->Name = new char[len + 1];
			newframe->Name[len] = 0;

			memcpy(newframe->Name, Name, len);
			//std::cout << "Added frame: " << newframe->Name << "\n";
		}
	} else {
		newframe->Name = nullptr;
	}

	newframe->pMeshContainer	= nullptr;
	newframe->pFrameSibling		= nullptr;
	newframe->pFrameFirstChild	= nullptr;
	newframe->exEnabled			= true;

	*retNewFrame = newframe;
	return D3D_OK;
}

HRESULT DXAnimatedMesh::CreateMeshContainer(
	LPCSTR Name,
	const D3DXMESHDATA* meshData,
	const D3DXMATERIAL* materials,
	const D3DXEFFECTINSTANCE* effectInstances,
	DWORD numMaterials,
	const DWORD* adjacency,
	LPD3DXSKININFO pSkinInfo,
	LPD3DXMESHCONTAINER* retNewMeshContainer)
{
	*retNewMeshContainer = 0;

	D3DXMESHCONTAINER_EXTENDED* newMeshContainer = new D3DXMESHCONTAINER_EXTENDED;
	memset(newMeshContainer, 0, sizeof(D3DXMESHCONTAINER_EXTENDED));

	if (meshData->Type != D3DXMESHTYPE_MESH) {
		DestroyMeshContainer(newMeshContainer);
		std::cout << "Only simple meshes are supported\n";

		return E_FAIL;
	}

	newMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;
	DWORD dwFaces = meshData->pMesh->GetNumFaces();

	newMeshContainer->pAdjacency = new DWORD[dwFaces * 3];
	memcpy(newMeshContainer->pAdjacency, adjacency, sizeof(DWORD) * dwFaces * 3);
	
	LPDIRECT3DDEVICE9 pd3dDevice = nullptr;
	meshData->pMesh->GetDevice(&pd3dDevice);

	if (Name) {
		size_t len = strlen(Name);

		if (len > 0) {
			newMeshContainer->Name = new char[len + 1];
			newMeshContainer->Name[len] = 0;

			memcpy(newMeshContainer->Name, Name, len);
			//std::cout << "Added mesh container: " << newMeshContainer->Name << "\n";
		} else {
			newMeshContainer->Name = nullptr;
		}
	} else {
		newMeshContainer->Name = nullptr;
	}

	newMeshContainer->MeshData.pMesh = meshData->pMesh;
	newMeshContainer->MeshData.pMesh->AddRef();

	newMeshContainer->NumMaterials = Math::Max<DWORD>(numMaterials, 1);
	newMeshContainer->exMaterials = new D3DMATERIAL9[newMeshContainer->NumMaterials];
	newMeshContainer->exTextures = new LPDIRECT3DTEXTURE9[newMeshContainer->NumMaterials];

	memset(newMeshContainer->exTextures, 0, sizeof(LPDIRECT3DTEXTURE9) * newMeshContainer->NumMaterials);

	if (numMaterials > 0) {
		for (DWORD i = 0; i < numMaterials; ++i) {
			newMeshContainer->exTextures[i] = 0;	
			newMeshContainer->exMaterials[i] = materials[i].MatD3D;

			if (materials[i].pTextureFilename) {
				std::string texturePath(materials[i].pTextureFilename);
				
				if (FAILED(D3DXCreateTextureFromFile(pd3dDevice, (path + texturePath).c_str(), &newMeshContainer->exTextures[i]))) {
					std::cout << "Could not load texture '" << texturePath << "'\n";
				}
			}
		}
	} else {
		// create default material
		memset(&newMeshContainer->exMaterials[0], 0, sizeof(D3DMATERIAL9));

		newMeshContainer->exMaterials[0].Diffuse.r = 0.5f;
		newMeshContainer->exMaterials[0].Diffuse.g = 0.5f;
		newMeshContainer->exMaterials[0].Diffuse.b = 0.5f;
		newMeshContainer->exMaterials[0].Specular = newMeshContainer->exMaterials[0].Diffuse;
		newMeshContainer->exTextures[0] = 0;
	}

	if (pSkinInfo) {
		newMeshContainer->pSkinInfo = pSkinInfo;
		pSkinInfo->AddRef();

		UINT numBones = pSkinInfo->GetNumBones();

		newMeshContainer->exBoneOffsets = new D3DXMATRIX[numBones];
		newMeshContainer->exFrameCombinedMatrixPointer = new D3DXMATRIX*[numBones];

		for (UINT i = 0; i < numBones; ++i)
			newMeshContainer->exBoneOffsets[i] = *(newMeshContainer->pSkinInfo->GetBoneOffsetMatrix(i));

		GenerateSkinnedMesh(newMeshContainer);
	} else {
		newMeshContainer->pSkinInfo = 0;
		newMeshContainer->exBoneOffsets = 0;
		newMeshContainer->exSkinMesh = 0;
		newMeshContainer->exFrameCombinedMatrixPointer = 0;
	}

	pd3dDevice->Release();

	if (effectInstances) {
		if (effectInstances->pEffectFilename)
			std::cout << "Effect instances not supported\n";
	}
	
	*retNewMeshContainer = newMeshContainer; 
	return D3D_OK;
}


HRESULT DXAnimatedMesh::DestroyFrame(LPD3DXFRAME frameToFree) 
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)frameToFree;

	if (frame->Name) {
		delete[] frame->Name;
		frame->Name = 0;
	}

	delete frame;
	return D3D_OK; 
}

HRESULT DXAnimatedMesh::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
	D3DXMESHCONTAINER_EXTENDED* meshContainer = (D3DXMESHCONTAINER_EXTENDED*)meshContainerBase;

	if (!meshContainer)
		return D3D_OK;

	if (meshContainer->Name) {
		delete[] meshContainer->Name;
		meshContainer->Name = nullptr;
	}

	if (meshContainer->exMaterials) {
		delete[] meshContainer->exMaterials;
		meshContainer->exMaterials = nullptr;
	}

	if (meshContainer->exTextures) {
		for (UINT i = 0; i < meshContainer->NumMaterials; ++i) {
			if (meshContainer->exTextures[i]) {
				meshContainer->exTextures[i]->Release();
				meshContainer->exTextures[i] = nullptr;
			}
		}
	}

	if (meshContainer->exTextures) {
		delete[] meshContainer->exTextures;
		meshContainer->exTextures = nullptr;
	}

	if (meshContainer->pAdjacency) {
		delete[] meshContainer->pAdjacency;
		meshContainer->pAdjacency = nullptr;
	}
	
	if (meshContainer->exBoneOffsets) {
		delete[] meshContainer->exBoneOffsets;
		meshContainer->exBoneOffsets = nullptr;
	}
	
	if (meshContainer->exFrameCombinedMatrixPointer) {
		delete[] meshContainer->exFrameCombinedMatrixPointer;
		meshContainer->exFrameCombinedMatrixPointer = nullptr;
	}
	
	if (meshContainer->exSkinMesh) {
		meshContainer->exSkinMesh->Release();
		meshContainer->exSkinMesh = nullptr;
	}
	
	if (meshContainer->MeshData.pMesh) {
		meshContainer->MeshData.pMesh->Release();
		meshContainer->MeshData.pMesh = nullptr;
	}
		
	if (meshContainer->pSkinInfo) {
		meshContainer->pSkinInfo->Release();
		meshContainer->pSkinInfo = nullptr;
	}
	
	delete meshContainer;
	meshContainer = nullptr;

	return D3D_OK;
}

void DXAnimatedMesh::GenerateSkinnedMesh(D3DXMESHCONTAINER_EXTENDED* meshContainer)
{
	if (method == SkinningMethodShader) {
		if (!meshContainer->MeshData.pMesh)
			return;

		DWORD Flags = D3DXMESHOPT_VERTEXCACHE|D3DXMESH_MANAGED;
		HRESULT hr;

		meshContainer->exNumBoneMatrices = Math::Min<DWORD>(MAX_BONE_MATRICES, meshContainer->pSkinInfo->GetNumBones());

		if (meshContainer->exSkinMesh) {
			meshContainer->exSkinMesh->Release();
			meshContainer->exSkinMesh = nullptr;
		}

		if (meshContainer->exBoneCombinationBuff) {
			meshContainer->exBoneCombinationBuff->Release();
			meshContainer->exBoneCombinationBuff = nullptr;
		}
		
		hr = meshContainer->pSkinInfo->ConvertToIndexedBlendedMesh(
			meshContainer->MeshData.pMesh,
			Flags,
			meshContainer->exNumBoneMatrices,
			meshContainer->pAdjacency,
			NULL, NULL, NULL,
			&meshContainer->exNumInfl,
			&meshContainer->exNumAttributeGroups,
			&meshContainer->exBoneCombinationBuff,
			&meshContainer->exSkinMesh);
		
		if (FAILED(hr)) {
			std::cout << "Could not convert to blended mesh\n";
			return;
		}

		// beta fields are used on the FFP
		DWORD NewFVF = (meshContainer->exSkinMesh->GetFVF() & D3DFVF_POSITION_MASK)|D3DFVF_NORMAL|D3DFVF_TEX1|D3DFVF_LASTBETA_UBYTE4;

		if (NewFVF != meshContainer->exSkinMesh->GetFVF()) {
			LPD3DXMESH pMesh = NULL;

			hr = meshContainer->exSkinMesh->CloneMeshFVF(
				meshContainer->exSkinMesh->GetOptions(), NewFVF, device, &pMesh);

			if (FAILED(hr)) {
				std::cout << "Could not clone FVF\n";
			} else {
				meshContainer->exSkinMesh->Release();
				meshContainer->exSkinMesh = pMesh;
			}
		}

		D3DVERTEXELEMENT9 pDecl[MAX_FVF_DECL_SIZE];
		LPD3DVERTEXELEMENT9 pDeclCur;

		hr = meshContainer->exSkinMesh->GetDeclaration(pDecl);

		if (FAILED(hr)) {
			std::cout << "Could not get declaration\n";
			return;
		}

		// some cards don't handle ubyte4
		pDeclCur = pDecl;

		while (pDeclCur->Stream != 0xff) {
			if ((pDeclCur->Usage == D3DDECLUSAGE_BLENDINDICES) && (pDeclCur->UsageIndex == 0))
				pDeclCur->Type = D3DDECLTYPE_D3DCOLOR;

			pDeclCur++;
		}

		hr = meshContainer->exSkinMesh->UpdateSemantics(pDecl);

		if (FAILED(hr)) {
			std::cout << "Could not update semantics\n";
			return;
		}

		maxnumbones = Math::Max<DWORD>(maxnumbones, (int)meshContainer->pSkinInfo->GetNumBones());
	} else {
		if (meshContainer->MeshData.pMesh) {
			D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];

			if (FAILED(meshContainer->MeshData.pMesh->GetDeclaration(decl)))
				return;

			meshContainer->MeshData.pMesh->CloneMesh(D3DXMESH_MANAGED, decl, device, &meshContainer->exSkinMesh);
		}

		maxnumbones = Math::Max<DWORD>(maxnumbones, (int)meshContainer->pSkinInfo->GetNumBones());
	}
}

HRESULT DXAnimatedMesh::Load(LPDIRECT3DDEVICE9 device, const std::string& file)
{
	if (!this->device)
		this->device = device;

	Math::GetPath(path, file);

	HRESULT hr = D3DXLoadMeshHierarchyFromX(file.c_str(), D3DXMESH_MANAGED, device, this, NULL, &root, &controller);

	if (SUCCEEDED(hr)) {
		if (controller) {
			numanimations = controller->GetNumAnimationSets();
			currentanim = 0;
			currenttrack = 0;

			std::cout << "Number of animations: " << numanimations << "\n";
		} else {
			std::cout << "No animation controller\n";
		}

		if (root) {
			SetupMatrices((D3DXFRAME_EXTENDED*)root, NULL);

			if (bonetransforms)
				delete[] bonetransforms;

			bonetransforms = new D3DXMATRIX[maxnumbones];
		}
	}

	return hr;
}

void DXAnimatedMesh::CloneMeshContainer(LPD3DXMESHCONTAINER from, LPD3DXMESHCONTAINER* to)
{
	D3DXMESHCONTAINER_EXTENDED*	exfrom				= (D3DXMESHCONTAINER_EXTENDED*)from;
	D3DXMESHCONTAINER_EXTENDED*	newMeshContainer	= new D3DXMESHCONTAINER_EXTENDED();
	DWORD						dwFaces				= from->MeshData.pMesh->GetNumFaces();

	*to = 0;
	memset(newMeshContainer, 0, sizeof(D3DXMESHCONTAINER_EXTENDED));

	newMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;
	newMeshContainer->pAdjacency = new DWORD[dwFaces * 3];
	
	memcpy(newMeshContainer->pAdjacency, from->pAdjacency, sizeof(DWORD) * dwFaces * 3);

	if (from->Name != nullptr) {
		size_t len = strlen(from->Name);

		if (len > 0) {
			newMeshContainer->Name = new char[len + 1];
			newMeshContainer->Name[len] = 0;

			memcpy(newMeshContainer->Name, from->Name, len);
			//std::cout << "Cloned mesh container: " << newMeshContainer->Name << "\n";
		} else {
			newMeshContainer->Name = nullptr;
		}
	} else {
		newMeshContainer->Name = nullptr;
	}

	newMeshContainer->MeshData.pMesh = from->MeshData.pMesh;
	from->MeshData.pMesh->AddRef();

	newMeshContainer->NumMaterials = from->NumMaterials;
	newMeshContainer->exMaterials = new D3DMATERIAL9[newMeshContainer->NumMaterials];
	newMeshContainer->exTextures = new LPDIRECT3DTEXTURE9[newMeshContainer->NumMaterials];

	memset(newMeshContainer->exTextures, 0, sizeof(LPDIRECT3DTEXTURE9) * newMeshContainer->NumMaterials);

	for (DWORD i = 0; i < from->NumMaterials; ++i) {
		newMeshContainer->exTextures[i]		= 0;
		newMeshContainer->exMaterials[i]	= exfrom->exMaterials[i];
		newMeshContainer->exTextures[i]		= exfrom->exTextures[i];

		exfrom->exTextures[i]->AddRef();
	}

	if (from->pSkinInfo) {
		newMeshContainer->pSkinInfo = from->pSkinInfo;
		from->pSkinInfo->AddRef();

		UINT numBones = from->pSkinInfo->GetNumBones();

		newMeshContainer->exBoneOffsets = new D3DXMATRIX[numBones];
		newMeshContainer->exFrameCombinedMatrixPointer = new D3DXMATRIX*[numBones];

		for (UINT i = 0; i < numBones; ++i)
			newMeshContainer->exBoneOffsets[i] = *(newMeshContainer->pSkinInfo->GetBoneOffsetMatrix(i));

		newMeshContainer->exNumBoneMatrices		= exfrom->exNumBoneMatrices;
		newMeshContainer->exNumInfl				= exfrom->exNumInfl;
		newMeshContainer->exNumAttributeGroups	= exfrom->exNumAttributeGroups;
		newMeshContainer->exBoneCombinationBuff	= exfrom->exBoneCombinationBuff;
		newMeshContainer->exSkinMesh			= exfrom->exSkinMesh;

		exfrom->exBoneCombinationBuff->AddRef();
		exfrom->exSkinMesh->AddRef();
	} else {
		newMeshContainer->pSkinInfo						= nullptr;
		newMeshContainer->exBoneOffsets					= nullptr;
		newMeshContainer->exSkinMesh					= nullptr;
		newMeshContainer->exFrameCombinedMatrixPointer	= nullptr;
	}

	if (from->pNextMeshContainer)
		CloneMeshContainer(from->pNextMeshContainer, &newMeshContainer->pNextMeshContainer);

	*to = newMeshContainer;
}

void DXAnimatedMesh::CloneFrames(LPD3DXFRAME from, LPD3DXFRAME fromparent, LPD3DXFRAME to, LPD3DXFRAME toparent)
{
	D3DXFRAME_EXTENDED* exfrom = (D3DXFRAME_EXTENDED*)from;
	D3DXFRAME_EXTENDED* exto = (D3DXFRAME_EXTENDED*)to;

	exto->exCombinedTransformationMatrix	= exfrom->exCombinedTransformationMatrix;
	exto->exEnabled							= exfrom->exEnabled;
	exto->TransformationMatrix				= exfrom->TransformationMatrix;
	exto->pMeshContainer					= 0;

	if (exfrom->pMeshContainer)
		CloneMeshContainer(exfrom->pMeshContainer, &exto->pMeshContainer);

	if (from->pFrameSibling) {
		LPD3DXFRAME tosibling = nullptr;

		CreateFrame(from->pFrameSibling->Name, &tosibling);
		D3DXFrameAppendChild(toparent, tosibling);

		CloneFrames(from->pFrameSibling, fromparent, tosibling, toparent);
	}

	if (from->pFrameFirstChild) {
		LPD3DXFRAME tochild = nullptr;

		CreateFrame(from->pFrameFirstChild->Name, &tochild);
		D3DXFrameAppendChild(to, tochild);

		CloneFrames(from->pFrameFirstChild, from, tochild, to);
	}
}

HRESULT DXAnimatedMesh::Clone(DXAnimatedMesh& to)
{
	to.method			= method;
	to.path				= path;
	to.effect			= effect;

	to.device			= device;
	to.maxnumbones		= maxnumbones;
	to.numanimations	= numanimations;
	to.currentanim		= currentanim;
	to.currenttrack		= currenttrack;

	controller->CloneAnimationController(
		controller->GetMaxNumAnimationOutputs(),
		controller->GetMaxNumAnimationSets(),
		controller->GetMaxNumTracks(),
		controller->GetMaxNumEvents(),
		&to.controller);

	CreateFrame(root->Name, &to.root);
	CloneFrames(root, 0, to.root, 0);

	D3DXFrameRegisterNamedMatrices(to.root, to.controller);
	to.SetupMatrices((D3DXFRAME_EXTENDED*)to.root, NULL);

	if (to.bonetransforms != nullptr)
		delete[] to.bonetransforms;

	to.bonetransforms = new D3DXMATRIX[maxnumbones];
	return D3D_OK;
}

void DXAnimatedMesh::SetupMatrices(D3DXFRAME_EXTENDED* frame, LPD3DXMATRIX parent)
{
	D3DXMESHCONTAINER_EXTENDED* pMeshContainer = (D3DXMESHCONTAINER_EXTENDED*)frame->pMeshContainer;

	if (pMeshContainer) {
		if (pMeshContainer->pSkinInfo) {
			for (DWORD i = 0; i < pMeshContainer->pSkinInfo->GetNumBones(); ++i) {
				D3DXFRAME_EXTENDED* pTempFrame =
					(D3DXFRAME_EXTENDED*)D3DXFrameFind(root, pMeshContainer->pSkinInfo->GetBoneName(i));

				pMeshContainer->exFrameCombinedMatrixPointer[i] = &pTempFrame->exCombinedTransformationMatrix;
			}
		}
	}

	if (frame->pFrameSibling)
		SetupMatrices((D3DXFRAME_EXTENDED*)frame->pFrameSibling, parent);

	if (frame->pFrameFirstChild)
		SetupMatrices((D3DXFRAME_EXTENDED*)frame->pFrameFirstChild, &frame->exCombinedTransformationMatrix);
}

void DXAnimatedMesh::UpdateMatrices(LPD3DXFRAME frame, LPD3DXMATRIX parent)
{
	D3DXFRAME_EXTENDED* currentFrame = (D3DXFRAME_EXTENDED*)frame;

	if (parent != nullptr) {
		D3DXMatrixMultiply(
			&currentFrame->exCombinedTransformationMatrix,
			&currentFrame->TransformationMatrix, parent);
	} else {
		currentFrame->exCombinedTransformationMatrix =
			currentFrame->TransformationMatrix;
	}

	if (currentFrame->pFrameSibling)
		UpdateMatrices(currentFrame->pFrameSibling, parent);

	if (currentFrame->pFrameFirstChild)
		UpdateMatrices(currentFrame->pFrameFirstChild, &currentFrame->exCombinedTransformationMatrix);
}

void DXAnimatedMesh::DrawFrame(LPD3DXFRAME frame)
{
	if (((D3DXFRAME_EXTENDED*)frame)->exEnabled) {
		LPD3DXMESHCONTAINER meshContainer = frame->pMeshContainer;

		while (meshContainer != nullptr) {
			DrawMeshContainer(meshContainer, frame);
			meshContainer = meshContainer->pNextMeshContainer;
		}

		if (frame->pFrameFirstChild)
			DrawFrame(frame->pFrameFirstChild);
	}

	if (frame->pFrameSibling)
		DrawFrame(frame->pFrameSibling);
}

void DXAnimatedMesh::DrawMeshContainer(LPD3DXMESHCONTAINER meshContainerBase, LPD3DXFRAME frameBase)
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)frameBase;
	D3DXMESHCONTAINER_EXTENDED* meshContainer = (D3DXMESHCONTAINER_EXTENDED*)meshContainerBase;

	if (method == SkinningMethodSoftware) {
		if (meshContainer->pSkinInfo != nullptr) {
			// skinned
			DWORD Bones = meshContainer->pSkinInfo->GetNumBones();
			
			for (DWORD i = 0; i < Bones; ++i)
				D3DXMatrixMultiply(&bonetransforms[i], &meshContainer->exBoneOffsets[i], meshContainer->exFrameCombinedMatrixPointer[i]);
			
			void* srcPtr = 0;
			void* destPtr = 0;

			meshContainer->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&srcPtr);
			meshContainer->exSkinMesh->LockVertexBuffer(0, (void**)&destPtr);
			{
				meshContainer->pSkinInfo->UpdateSkinnedMesh(bonetransforms, NULL, srcPtr, destPtr);
			}
			meshContainer->exSkinMesh->UnlockVertexBuffer();
			meshContainer->MeshData.pMesh->UnlockVertexBuffer();

			D3DXMATRIX id(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
			device->SetTransform(D3DTS_WORLD, &id);

			for (DWORD iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial) {
				device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
				device->SetTexture(0, meshContainer->exTextures[iMaterial]);

				LPD3DXMESH pDrawMesh = meshContainer->exSkinMesh;
				pDrawMesh->DrawSubset(iMaterial);
			}
		} else {
			// regular
			device->SetTransform(D3DTS_WORLD, &frame->exCombinedTransformationMatrix);

			for (DWORD iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial) {
				device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
				device->SetTexture(0, meshContainer->exTextures[iMaterial]);

				LPD3DXMESH pDrawMesh = meshContainer->MeshData.pMesh;
				pDrawMesh->DrawSubset(iMaterial);
			}
		}
	} else if (method == SkinningMethodShader) {
		if (meshContainer->exBoneCombinationBuff != nullptr) {
			// skinned
			if (effect == nullptr)
				return;

			LPD3DXBONECOMBINATION pBoneComb =
				reinterpret_cast<LPD3DXBONECOMBINATION>(meshContainer->exBoneCombinationBuff->GetBufferPointer());

			for (DWORD iAttrib = 0; iAttrib < meshContainer->exNumAttributeGroups; ++iAttrib) {
				for (DWORD iPaletteEntry = 0; iPaletteEntry < meshContainer->exNumBoneMatrices; ++iPaletteEntry) {
					DWORD iMatrixIndex = pBoneComb[iAttrib].BoneId[iPaletteEntry];

					if (iMatrixIndex != UINT_MAX) {
						D3DXMatrixMultiply(
							&bonetransforms[iPaletteEntry],
							&meshContainer->exBoneOffsets[iMatrixIndex],
							meshContainer->exFrameCombinedMatrixPointer[iMatrixIndex]);
					}
				}

				effect->SetTechnique("skinned");

				effect->SetMatrixArray("matBones", bonetransforms, meshContainer->exNumBoneMatrices);
				effect->SetInt("numBones", meshContainer->exNumInfl - 1);

				device->SetTexture(0, meshContainer->exTextures[pBoneComb[iAttrib].AttribId]);

				effect->Begin(NULL, D3DXFX_DONOTSAVESTATE);
				effect->BeginPass(0);
				{
					meshContainer->exSkinMesh->DrawSubset(iAttrib);
				}
				effect->EndPass();
				effect->End();
			}
		} else {
			// regular
			effect->SetTechnique("regular");
			effect->SetMatrix("matWorld", &frame->exCombinedTransformationMatrix);

			effect->Begin(NULL, D3DXFX_DONOTSAVESTATE);
			effect->BeginPass(0);
			{
				for (DWORD iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial) {
					device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
					device->SetTexture(0, meshContainer->exTextures[iMaterial]);

					LPD3DXMESH pDrawMesh = meshContainer->MeshData.pMesh;
					pDrawMesh->DrawSubset(iMaterial);
				}
			}
			effect->EndPass();
			effect->End();
		}
	}
}

void DXAnimatedMesh::SetSkinningMethod(SkinningMethod newmethod, LPD3DXEFFECT neweffect)
{
	method = newmethod;

	if (effect != nullptr)
		effect->Release();

	if (newmethod == SkinningMethodShader) {
		assert(neweffect != nullptr);

		if (neweffect != effect) {
			effect = neweffect;
			effect->AddRef();
		}
	} else {
		effect = nullptr;
	}
}

void DXAnimatedMesh::SetAnimation(UINT index)
{
	if (index >= numanimations || index == currentanim)
		return;

	LPD3DXANIMATIONSET set = NULL;
	UINT newtrack = (currenttrack == 0 ? 1 : 0);
	double transitiontime = 0.25;

	controller->GetAnimationSet(index, &set);
	controller->SetTrackAnimationSet(newtrack, set);

	set->Release();

	controller->UnkeyAllTrackEvents(currenttrack);
	controller->UnkeyAllTrackEvents(newtrack);

	// smooth blending between animations
	controller->KeyTrackEnable(currenttrack, false, currenttime + transitiontime);
	controller->KeyTrackSpeed(currenttrack, 0.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);
	controller->KeyTrackWeight(currenttrack, 0.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);

	controller->SetTrackEnable(newtrack, true);
	controller->KeyTrackSpeed(newtrack, 1.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);
	controller->KeyTrackWeight(newtrack, 1.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);

	currenttrack = newtrack;
	currentanim = index;
}

void DXAnimatedMesh::Update(float delta, LPD3DXMATRIX world)
{
	if (controller != nullptr)
		controller->AdvanceTime(delta, NULL);

	if (root != nullptr)
		UpdateMatrices(root, world);

	currenttime += delta;
}

void DXAnimatedMesh::Draw()
{
	if (root != nullptr)
		DrawFrame(root);
}

void DXAnimatedMesh::EnableFrame(const std::string& name, bool enable)
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)D3DXFrameFind(root, name.c_str());

	if (frame != nullptr)
		frame->exEnabled = enable;
}

void DXAnimatedMesh::NextAnimation()
{
	SetAnimation((currentanim + 1) % numanimations);
}

// --- DXLight impl -----------------------------------------------------------

DXLight::DXLight(LightType type, const D3DXVECTOR4& position, const D3DXCOLOR& color)
{
	this->type				= type;
	this->position			= position;
	this->color				= color;

	projparams				= D3DXVECTOR4(0, 0, 0, 0);
	spotdirection			= D3DXVECTOR3(0, 0, 0);
	spotparams				= D3DXVECTOR2(0, 0);

	cubeshadowmap			= nullptr;
	blurredcubeshadowmap	= nullptr;
	shadowmap				= nullptr;
	blurredshadowmap		= nullptr;
	currentface				= 0;
	shadowmapsize			= 0;
	blurred					= false;

	switch (type) {
	case LightTypePoint:
		this->position.w = 1;
		break;

	case LightTypeDirectional:
		this->position.w = 0;
		break;

	case LightTypeSpot:
		// haha :)
		this->position.w = 0.75f;
		break;

	default:
		break;
	}
}

DXLight::~DXLight()
{
	if (cubeshadowmap != nullptr)
		cubeshadowmap->Release();

	if (blurredcubeshadowmap != nullptr)
		blurredcubeshadowmap->Release();

	if (shadowmap != nullptr)
		shadowmap->Release();

	if (blurredshadowmap != nullptr)
		blurredshadowmap->Release();
}

void DXLight::CalculateViewProjection(D3DXMATRIX& out)
{
	if (type == LightTypeDirectional) {
		D3DXMATRIX proj;
		D3DXVECTOR3 eye = position;
		D3DXVECTOR3 look(0, 0, 0);
		D3DXVECTOR3 up(0, 1, 0);
		
		if (fabs(position.y) > 0.999f)
			up = D3DXVECTOR3(1, 0, 0);

		D3DXMatrixLookAtLH(&out, &eye, &look, &up);
		D3DXMatrixOrthoLH(&proj, projparams.x, projparams.y, projparams.z, projparams.w);
		D3DXMatrixMultiply(&out, &out, &proj);
	} else if (type == LightTypePoint) {
		D3DXMATRIX proj;
		D3DXVECTOR3 eye = position;

		D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 2, 1, projparams.z, projparams.w);
		D3DXMatrixLookAtLH(&out, &eye, &(eye + DXCubeForward[currentface]), &DXCubeUp[currentface]);
		D3DXMatrixMultiply(&out, &out, &proj);
	} else if (type == LightTypeSpot) {
		// TODO:
	}
}

void DXLight::CalculateScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj, float radius, int32_t width, int32_t height)
{
	// see https://www.gamasutra.com/view/feature/131351/the_mechanics_of_robust_stencil_.php?page=6

	if (type != LightTypePoint)
		return;

	D3DXVECTOR4 Q;
	D3DXVECTOR3 L, P1, P2;
	float u, v;

	out.left	= 0;
	out.right	= width;
	out.top		= 0;
	out.bottom	= height;

	D3DXVec3TransformCoord(&L, (D3DXVECTOR3*)&position, &view);

	float D = 4 * (radius * radius * L.x * L.x - (L.x * L.x + L.z * L.z) * (radius * radius - L.z * L.z));

	if (D > 0.0f) {
		float Nx1 = (radius * L.x + sqrtf(D * 0.25f)) / (L.x * L.x + L.z * L.z);
		float Nx2 = (radius * L.x - sqrtf(D * 0.25f)) / (L.x * L.x + L.z * L.z);
		float Nz1 = (radius - Nx1 * L.x) / L.z;
		float Nz2 = (radius - Nx2 * L.x) / L.z;

		P1.x = L.x - Nx1 * radius;
		P1.y = 0.0f;
		P1.z = L.z - Nz1 * radius;

		P2.x = L.x - Nx2 * radius;
		P2.y = 0.0f;
		P2.z = L.z - Nz2 * radius;

		// left-handed: >
		if (P1.z < 0.0f && P2.z < 0.0f) {
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.x * 0.5f + 0.5f) * width;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.x * 0.5f + 0.5f) * width;

			if (P2.x < L.x) {
				out.left = (LONG)max(v, 0.0f);
				out.right = (LONG)min(u, width);
			} else {
				out.left = (LONG)max(u, 0.0f);
				out.right = (LONG)min(v, width);
			}
		}
	}

	D = 4 * (radius * radius * L.y * L.y - (L.y * L.y + L.z * L.z) * (radius * radius - L.z * L.z));

	if (D >= 0.0f) {
		float Ny1 = (radius * L.y + sqrtf(D * 0.25f)) / (L.y * L.y + L.z * L.z);
		float Ny2 = (radius * L.y - sqrtf(D * 0.25f)) / (L.y * L.y + L.z * L.z);
		float Nz1 = (radius - Ny1 * L.y) / L.z;
		float Nz2 = (radius - Ny2 * L.y) / L.z;

		P1.x = 0.0f;
		P1.y = L.y - Ny1 * radius;
		P1.z = L.z - Nz1 * radius;

		P2.x = 0.0f;
		P2.y = L.y - Ny2 * radius;
		P2.z = L.z - Nz2 * radius;

		// left-handed: >
		if (P1.z < 0.0f && P2.z < 0.0f) {
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.y * -0.5f + 0.5f) * height;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.y * -0.5f + 0.5f) * height;

			if (P2.y < L.y) {
				out.top = (LONG)max(u, 0.0f);
				out.bottom = (LONG)min(v, height);
			} else {
				out.top = (LONG)max(v, 0.0f);
				out.bottom = (LONG)min(u, height);
			}
		}
	}
}

void DXLight::CreateShadowMap(LPDIRECT3DDEVICE9 device, uint16_t size)
{
	shadowmapsize = size;

	if (type == LightTypeDirectional) {
		device->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL);
		device->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blurredshadowmap, NULL);
	} else if (type == LightTypePoint) {
		device->CreateCubeTexture(size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &cubeshadowmap, NULL);
		device->CreateCubeTexture(size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blurredcubeshadowmap, NULL);
	}
}

void DXLight::RenderShadowMap(LPDIRECT3DDEVICE9 device, std::function<void (DXLight*)> callback)
{
	D3DVIEWPORT9 oldviewport;
	D3DVIEWPORT9 viewport;

	LPDIRECT3DSURFACE9 oldsurface = NULL;
	LPDIRECT3DSURFACE9 surface = NULL;

	device->GetRenderTarget(0, &oldsurface);
	device->GetViewport(&oldviewport);

	if (type == LightTypeDirectional) {
		shadowmap->GetSurfaceLevel(0, &surface);

		viewport.X = viewport.Y = 0;
		viewport.Width = viewport.Height = shadowmapsize;
		viewport.MinZ = 0;
		viewport.MaxZ = 1;

		device->SetRenderTarget(0, surface);
		device->SetViewport(&viewport);
		{
			callback(this);
		}
		surface->Release();
	} else if (type == LightTypePoint) {
		for (currentface = 0; currentface < 6; ++currentface) {
			cubeshadowmap->GetCubeMapSurface((D3DCUBEMAP_FACES)currentface, 0, &surface);

			viewport.X = viewport.Y = 0;
			viewport.Width = viewport.Height = shadowmapsize;
			viewport.MinZ = 0;
			viewport.MaxZ = 1;

			device->SetRenderTarget(0, surface);
			device->SetViewport(&viewport);
			{
				callback(this);
			}
			surface->Release();
		}
	}

	blurred = false;

	device->SetRenderTarget(0, oldsurface);
	device->SetViewport(&oldviewport);

	oldsurface->Release();
}

void DXLight::BlurShadowMap(LPDIRECT3DDEVICE9 device, std::function<void (DXLight*)> callback)
{
	D3DVIEWPORT9 oldviewport;
	D3DVIEWPORT9 viewport;

	LPDIRECT3DSURFACE9 oldsurface = NULL;
	LPDIRECT3DSURFACE9 surface = NULL;

	device->GetRenderTarget(0, &oldsurface);
	device->GetViewport(&oldviewport);

	if (type == LightTypeDirectional) {
		blurredshadowmap->GetSurfaceLevel(0, &surface);

		viewport.X = viewport.Y = 0;
		viewport.Width = viewport.Height = shadowmapsize;
		viewport.MinZ = 0;
		viewport.MaxZ = 1;

		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetRenderTarget(0, surface);
		device->SetViewport(&viewport);
		device->SetTexture(0, shadowmap);
		{
			callback(this);
		}
		surface->Release();
	} else {
		// TODO:
	}

	blurred = true;

	device->SetRenderTarget(0, oldsurface);
	device->SetViewport(&oldviewport);

	oldsurface->Release();
}

void DXLight::SetProjectionParameters(float xsize, float ysize, float znear, float zfar)
{
	projparams.x = xsize;
	projparams.y = ysize;
	projparams.z = znear;
	projparams.w = zfar;
}

void DXLight::SetSpotParameters(const D3DXVECTOR3& dir, float inner, float outer)
{
	spotdirection = dir;

	spotparams.x = cosf(inner);
	spotparams.y = cosf(outer);
}

// --- DXObject impl ----------------------------------------------------------

DXObject::DXObject(LPDIRECT3DDEVICE9 device)
{
	this->device	= device;
	mesh			= nullptr;
	basecolors		= nullptr;
	normalmaps		= nullptr;
	specularmaps	= nullptr;
	materials		= nullptr;
	nummaterials	= 0;
}

DXObject::~DXObject()
{
	Tidy();
}

void DXObject::Tidy()
{
	if (mesh != nullptr)
		mesh->Release();

	if (basecolors != nullptr) {
		for (DWORD i = 0; i < nummaterials; ++i) {
			if (basecolors[i] != nullptr)
				basecolors[i]->Release();
		}

		delete[] basecolors;
	}

	if (normalmaps != nullptr) {
		for (DWORD i = 0; i < nummaterials; ++i) {
			if (normalmaps[i] != nullptr)
				normalmaps[i]->Release();
		}

		delete[] normalmaps;
	}

	if (specularmaps != nullptr) {
		for (DWORD i = 0; i < nummaterials; ++i) {
			if (specularmaps[i] != nullptr)
				specularmaps[i]->Release();
		}

		delete[] specularmaps;
	}

	if (materials != nullptr) {
		for (DWORD i = 0; i < nummaterials; ++i) {
			if (materials[i].pTextureFilename != nullptr)
				delete[] materials[i].pTextureFilename;
		}

		delete[] materials;
	}

	mesh			= nullptr;
	basecolors		= nullptr;
	normalmaps		= nullptr;
	specularmaps	= nullptr;
	materials		= nullptr;
}

bool DXObject::Create(std::function<bool (LPD3DXMESH*)> callback)
{
	LPD3DXMESH newmesh = nullptr;

	if (callback(&newmesh)) {
		if (mesh)
			mesh->Release();

		newmesh->GetAttributeTable(nullptr, &nummaterials);
		mesh = newmesh;

		basecolors = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(basecolors, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		normalmaps = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(normalmaps, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		specularmaps = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(specularmaps, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		return true;
	}

	return false;
}

bool DXObject::GenerateTangentFrame()
{
	return SUCCEEDED(DXGenTangentFrame(device, mesh, &mesh));
}

bool DXObject::Load(const std::string& file)
{
	HRESULT			hr		= E_FAIL;
	LPD3DXBUFFER	buffer	= nullptr;
	std::string		path;
	std::string		ext;
	char*			matname	= nullptr;
	size_t			namelen	= 0;

	size_t pos = file.find_last_of("\\/");

	if (pos != std::string::npos)
		path = file.substr(0, pos + 1);

	pos = file.find_last_of(".");

	if (pos != std::string::npos)
		ext = file.substr(pos + 1);

	Tidy();

	if (ext == "x" || ext == "X")
		hr = D3DXLoadMeshFromX(file.c_str(), D3DXMESH_MANAGED, device, NULL, &buffer, NULL, &nummaterials, &mesh);
	else if( ext == "qm" )
		hr = DXLoadMeshFromQM(file.c_str(), D3DXMESH_MANAGED, device, &materials, &nummaterials, &mesh);
	else
		hr = E_FAIL;

	if (FAILED(hr))
		return false;

	if (nummaterials > 0) {
		if (buffer != nullptr) {
			materials = new D3DXMATERIAL[nummaterials];
			memcpy(materials, buffer->GetBufferPointer(), nummaterials * sizeof(D3DXMATERIAL));
		}

		basecolors = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(basecolors, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		normalmaps = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(normalmaps, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		specularmaps = new LPDIRECT3DTEXTURE9[nummaterials];
		memset(specularmaps, 0, nummaterials * sizeof(LPDIRECT3DTEXTURE9));

		for (DWORD i = 0; i < nummaterials; ++i) {
			if (materials[i].pTextureFilename != nullptr) {
				if (buffer != nullptr) {
					namelen = strlen(materials[i].pTextureFilename);
					matname = new char[namelen + 1];
			
					memcpy(matname, materials[i].pTextureFilename, namelen);
					matname[namelen] = 0;

					materials[i].pTextureFilename = matname;
				} else {
					matname = materials[i].pTextureFilename;
				}

				D3DXCreateTextureFromFileA(device, (path + matname).c_str(), &basecolors[i]);
			}
		}
	}

	if (buffer != nullptr)
		buffer->Release();

	return true;
}

bool DXObject::AddBaseTexture(const std::string& file, const std::vector<DWORD>& subsets)
{
	LPDIRECT3DTEXTURE9 texture = nullptr;

	if (FAILED(D3DXCreateTextureFromFile(device, file.c_str(), &texture)))
		return false;

	if (subsets.size() == 0) {
		// all subsets
		for (DWORD i = 0; i < nummaterials; ++i) {
			basecolors[i] = texture;
			texture->AddRef();
		}
	} else {
		for (DWORD i : subsets) {
			basecolors[i] = texture;
			texture->AddRef();
		}
	}

	texture->Release();
	return true;
}

bool DXObject::AddNormalMap(const std::string& file, const std::vector<DWORD>& subsets)
{
	LPDIRECT3DTEXTURE9 texture = nullptr;

	if (FAILED(D3DXCreateTextureFromFile(device, file.c_str(), &texture)))
		return false;

	if (subsets.size() == 0) {
		// all subsets
		for (DWORD i = 0; i < nummaterials; ++i) {
			normalmaps[i] = texture;
			texture->AddRef();
		}
	} else {
		for (DWORD i : subsets) {
			normalmaps[i] = texture;
			texture->AddRef();
		}
	}

	texture->Release();
	return true;
}

bool DXObject::AddSpecularMap(const std::string& file, const std::vector<DWORD>& subsets)
{
	LPDIRECT3DTEXTURE9 texture = nullptr;

	if (FAILED(D3DXCreateTextureFromFile(device, file.c_str(), &texture)))
		return false;

	if (subsets.size() == 0) {
		// all subsets
		for (DWORD i = 0; i < nummaterials; ++i) {
			specularmaps[i] = texture;
			texture->AddRef();
		}
	} else {
		for (DWORD i : subsets) {
			specularmaps[i] = texture;
			texture->AddRef();
		}
	}

	texture->Release();
	return true;
}

void DXObject::Draw(std::function<bool (DXObject*, DWORD)> callback)
{
	for (DWORD i = 0; i < nummaterials; ++i)
		DrawSubset(i, callback);
}

void DXObject::DrawSubset(DWORD subset, std::function<bool (DXObject*, DWORD)> callback)
{
	assert(subset < nummaterials);

	if (callback == nullptr || callback(this, subset))
		mesh->DrawSubset(subset);
}

void DXObject::DrawExcept(const std::vector<DWORD>& subsets, std::function<bool (DXObject*, DWORD)> callback)
{
	// TODO:
}

LPDIRECT3DTEXTURE9 DXObject::GetBaseTexture(DWORD subset)
{
	assert(subset < nummaterials);
	return basecolors[subset];
}

LPDIRECT3DTEXTURE9 DXObject::GetNormalMap(DWORD subset)
{
	assert(subset < nummaterials);
	return normalmaps[subset];
}

LPDIRECT3DTEXTURE9 DXObject::GetSpecularMap(DWORD subset)
{
	assert(subset < nummaterials);
	return specularmaps[subset];
}

const D3DCOLORVALUE& DXObject::GetBaseColor(DWORD subset) const
{
	assert(subset < nummaterials);
	return materials[subset].MatD3D.Diffuse;
}

// --- DXParticleStorage impl -------------------------------------------------

DXParticleStorage::DXParticleStorage(LPDIRECT3DDEVICE9 device)
{
	this->device = device;

	vertexbuffer = nullptr;
	vertexstride = 0;
	vertexcount = 0;
}

DXParticleStorage::~DXParticleStorage()
{
	if (vertexbuffer != nullptr) {
		vertexbuffer->Release();
		vertexbuffer = nullptr;
	}
}

bool DXParticleStorage::Initialize(size_t count, size_t stride)
{
	HRESULT hr = D3D_OK;

	if (vertexcount != count || vertexstride != stride) {
		if (vertexbuffer != nullptr)
			vertexbuffer->Release();

		hr = device->CreateVertexBuffer((UINT)(count * stride * 6), D3DUSAGE_DYNAMIC, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1, D3DPOOL_DEFAULT, &vertexbuffer, NULL);
	}

	vertexstride = stride;
	vertexcount = count;

	return SUCCEEDED(hr);
}

void* DXParticleStorage::LockVertexBuffer(uint32_t offset, uint32_t size)
{
	void* data = nullptr;
	HRESULT hr = vertexbuffer->Lock(offset, size, &data, D3DLOCK_DISCARD);

	if (FAILED(hr))
		return nullptr;

	return data;
}

void DXParticleStorage::UnlockVertexBuffer()
{
	vertexbuffer->Unlock();
}
