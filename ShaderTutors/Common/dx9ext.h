
#ifndef _DX9EXT_H_
#define _DX9EXT_H_

#include <d3dx9.h>
#include <gdiplus.h>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cassert>

#include "particlesystem.h"

#define D3DCOLORWRITEENABLE_ALL	(D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA)

// NOTE: everything here is left handed as that is Direct3D default
// NOTE: works seamlessly together with my own math lib (just know when to flip culling)

extern float DXScreenQuadVertices[24];
extern float DXScreenQuadVerticesFFP[24];

extern D3DXVECTOR3 DXCubeForward[6];
extern D3DXVECTOR3 DXCubeUp[6];

// --- Functions --------------------------------------------------------------

HRESULT DXCreateEffect(LPDIRECT3DDEVICE9 device, LPCTSTR file, LPD3DXEFFECT* out);
HRESULT DXLoadMeshFromQM(LPCTSTR file, DWORD options, LPDIRECT3DDEVICE9 device, D3DXMATERIAL** materials, DWORD* nummaterials, LPD3DXMESH* mesh);
HRESULT DXCreateChecker(LPDIRECT3DDEVICE9 device, UINT xseg, UINT yseg, DWORD color1, DWORD color2, LPDIRECT3DTEXTURE9* texture);
HRESULT DXCreatePlane(LPDIRECT3DDEVICE9 device, float width, float depth, LPD3DXMESH* mesh);
HRESULT DXGenTangentFrame(LPDIRECT3DDEVICE9 device, LPD3DXMESH mesh, LPD3DXMESH* newmesh);
HRESULT DXRenderTextEx(LPDIRECT3DDEVICE9 device, const std::string& str, uint32_t width, uint32_t height, const WCHAR* face, bool border, int style, float emsize, LPDIRECT3DTEXTURE9* texture);

// --- Enums ------------------------------------------------------------------

enum SkinningMethod
{
	SkinningMethodSoftware = 0,
	SkinningMethodShader
};

enum LightType
{
	LightTypePoint = 0,
	LightTypeDirectional,
	LightTypeSpot
};

// --- Extended structures ----------------------------------------------------

struct D3DXMESHCONTAINER_EXTENDED : D3DXMESHCONTAINER
{
	IDirect3DTexture9**	exTextures;
	D3DMATERIAL9*		exMaterials;
	ID3DXMesh*			exSkinMesh;
	D3DXMATRIX*			exBoneOffsets;
	D3DXMATRIX**		exFrameCombinedMatrixPointer;
	DWORD				exNumBoneMatrices;
	DWORD				exNumAttributeGroups;
	DWORD				exNumInfl;
	LPD3DXBUFFER		exBoneCombinationBuff;
};

struct D3DXFRAME_EXTENDED : D3DXFRAME
{
	D3DXMATRIX exCombinedTransformationMatrix;
	bool exEnabled;
};

// --- Classes ----------------------------------------------------------------

class DXAnimatedMesh : public ID3DXAllocateHierarchy
{
protected:
	std::string					path;				// to load textures
	LPDIRECT3DDEVICE9			device;
	LPD3DXEFFECT				effect;				// for shader skinning
	LPD3DXANIMATIONCONTROLLER	controller;
	LPD3DXFRAME					root;
	D3DXMATRIX*					bonetransforms;		// total transformation for bones
	DWORD						maxnumbones;
	UINT						numanimations;
	UINT						currentanim;
	UINT						currenttrack;
	double						currenttime;
	SkinningMethod				method;

	void CloneFrames(LPD3DXFRAME from, LPD3DXFRAME fromparent, LPD3DXFRAME to, LPD3DXFRAME toparent);
	void CloneMeshContainer(LPD3DXMESHCONTAINER from, LPD3DXMESHCONTAINER* to);
	void GenerateSkinnedMesh(D3DXMESHCONTAINER_EXTENDED* meshContainer);
	void SetupMatrices(D3DXFRAME_EXTENDED* frame, LPD3DXMATRIX parent);
	void UpdateMatrices(LPD3DXFRAME frame, LPD3DXMATRIX parent);
	void DrawFrame(LPD3DXFRAME frame);
	void DrawMeshContainer(LPD3DXMESHCONTAINER meshContainerBase, LPD3DXFRAME frameBase);

public:
	STDMETHOD(CreateMeshContainer)(
		LPCSTR Name, const D3DXMESHDATA* meshData, const D3DXMATERIAL* materials, const D3DXEFFECTINSTANCE* effectInstances,
		DWORD numMaterials, const DWORD* adjacency, LPD3DXSKININFO skinInfo, LPD3DXMESHCONTAINER* retNewMeshContainer );

	STDMETHOD(CreateFrame)(LPCSTR Name, LPD3DXFRAME* retNewFrame);
	STDMETHOD(DestroyFrame)(LPD3DXFRAME frameToFree);
	STDMETHOD(DestroyMeshContainer)(LPD3DXMESHCONTAINER meshContainerToFree);

	DXAnimatedMesh();
	~DXAnimatedMesh();

	HRESULT Load(LPDIRECT3DDEVICE9 device, const std::string& file);
	HRESULT Clone(DXAnimatedMesh& to);

	void SetSkinningMethod(SkinningMethod newmethod, LPD3DXEFFECT neweffect = nullptr);
	void SetAnimation(UINT index);
	void NextAnimation();
	void Update(float delta, LPD3DXMATRIX world);
	void Draw();
	void EnableFrame(const std::string& name, bool enable);
};

class DXLight
{
private:
	D3DXVECTOR4				position;	// or direction
	D3DXVECTOR4				projparams;
	D3DXVECTOR3				spotdirection;
	D3DXVECTOR2				spotparams;	// cos(inner), cos(outer)

	D3DXCOLOR				color;
	LPDIRECT3DCUBETEXTURE9	cubeshadowmap;
	LPDIRECT3DCUBETEXTURE9	blurredcubeshadowmap;
	LPDIRECT3DTEXTURE9		shadowmap;
	LPDIRECT3DTEXTURE9		blurredshadowmap;
	LightType				type;
	int						currentface;
	uint16_t				shadowmapsize;
	bool					blurred;

public:
	DXLight(LightType type, const D3DXVECTOR4& position, const D3DXCOLOR& color);
	~DXLight();

	void CalculateViewProjection(D3DXMATRIX& out);
	void CalculateScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj, float radius, int32_t width, int32_t height);

	void CreateShadowMap(LPDIRECT3DDEVICE9 device, uint16_t size);
	void RenderShadowMap(LPDIRECT3DDEVICE9 device, std::function<void (DXLight*)> callback);
	void BlurShadowMap(LPDIRECT3DDEVICE9 device, std::function<void (DXLight*)> callback);
	void SetProjectionParameters(float xsize, float ysize, float znear, float zfar);
	void SetSpotParameters(const D3DXVECTOR3& dir, float inner, float outer);

	inline void SetPosition(const D3DXVECTOR4& newpos)	{ position = newpos; }

	inline float GetNearPlane() const					{ return projparams.z; }
	inline float GetFarPlane() const					{ return projparams.w; }
	inline uint16_t GetShadowMapSize() const			{ return shadowmapsize; }
	inline const D3DXVECTOR2& GetSpotParameters() const	{ return spotparams; }

	inline D3DXVECTOR4& GetPosition()					{ return position; }
	inline D3DXVECTOR3& GetSpotDirection()				{ return spotdirection; }
	inline D3DXCOLOR& GetColor()						{ return color; }

	inline LPDIRECT3DTEXTURE9 GetShadowMap()			{ return (blurred ? blurredshadowmap : shadowmap); }
	inline LPDIRECT3DCUBETEXTURE9 GetCubeShadowMap()	{ return (blurred ? blurredcubeshadowmap : cubeshadowmap); }
};

class DXObject
{
private:
	LPDIRECT3DDEVICE9	device;
	LPD3DXMESH			mesh;
	D3DXMATERIAL*		materials;
	LPDIRECT3DTEXTURE9*	basecolors;
	LPDIRECT3DTEXTURE9*	normalmaps;
	LPDIRECT3DTEXTURE9*	specularmaps;
	DWORD				nummaterials;

	void Tidy();

public:
	DXObject(LPDIRECT3DDEVICE9 device);
	~DXObject();

	bool Create(std::function<bool (LPD3DXMESH*)> callback);
	bool GenerateTangentFrame();
	bool Load(const std::string& file);
	bool AddBaseTexture(const std::string& file, const std::vector<DWORD>& subsets = {});
	bool AddNormalMap(const std::string& file, const std::vector<DWORD>& subsets = {});
	bool AddSpecularMap(const std::string& file, const std::vector<DWORD>& subsets = {});

	void Draw(std::function<bool (DXObject*, DWORD)> callback);
	void DrawSubset(DWORD subset, std::function<bool (DXObject*, DWORD)> callback);
	void DrawExcept(const std::vector<DWORD>& subsets, std::function<bool (DXObject*, DWORD)> callback);

	LPDIRECT3DTEXTURE9 GetBaseTexture(DWORD subset);
	LPDIRECT3DTEXTURE9 GetNormalMap(DWORD subset);
	LPDIRECT3DTEXTURE9 GetSpecularMap(DWORD subset);
	const D3DCOLORVALUE& GetBaseColor(DWORD subset) const;
};

class DXParticleStorage : public IParticleStorage
{
private:
	LPDIRECT3DDEVICE9		device;
	LPDIRECT3DVERTEXBUFFER9	vertexbuffer;
	size_t					vertexstride;
	size_t					vertexcount;

public:
	DXParticleStorage(LPDIRECT3DDEVICE9 device);
	~DXParticleStorage();

	bool Initialize(size_t count, size_t stride) override;
	void* LockVertexBuffer(uint32_t offset, uint32_t size) override;
	void UnlockVertexBuffer() override;

	inline LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer()	{ return vertexbuffer; }
	inline size_t GetVertexStride() const override		{ return vertexstride; }
	inline size_t GetNumVertices() const override		{ return vertexcount; }
};

#endif
