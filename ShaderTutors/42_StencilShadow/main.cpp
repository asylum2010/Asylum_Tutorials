
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"
#include "..\Common\geometryutils.h"

#define MANIFOLD_EPSILON	8e-3f	// to avoid zfight

// helper macros
#define TITLE				"Shader sample 42: Shadow volumes"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample classes
class ShadowCaster
{
	typedef GeometryUtils::EdgeSet EdgeSet;
	typedef std::vector<GeometryUtils::Edge> EdgeList;

private:
	Math::Matrix			transform;
	EdgeSet					edges;
	EdgeList				silhouette;

	LPD3DXMESH				mesh;			// for rendering
	LPD3DXMESH				manifold;		// for shadow generation
	LPDIRECT3DVERTEXBUFFER9	volumevertices;
	LPDIRECT3DINDEXBUFFER9	volumeindices;

	UINT					numvolumevertices;
	UINT					numvolumeindices;

	void GenerateSilhouette(const Math::Vector4& position);

public:
	ShadowCaster();
	~ShadowCaster();

	void CreateGeometry(std::function<void (LPD3DXMESH*, LPD3DXMESH*)> callback);
	void ExtrudeSilhouette(const Math::Vector4& position);
	void SetTransform(const Math::Matrix& transform);

	inline const Math::Matrix& GetTransform() const		{ return transform; }
	inline const EdgeList& GetSilhouette() const		{ return silhouette; }

	inline LPD3DXMESH GetMesh()							{ return mesh; }
	inline LPD3DXMESH GetManifold()						{ return manifold; }

	inline LPDIRECT3DVERTEXBUFFER9 GetVolumeVertices()	{ return volumevertices; }
	inline LPDIRECT3DINDEXBUFFER9 GetVolumeIndices()	{ return volumeindices; }

	inline UINT GetNumVolumeVertices() const			{ return numvolumevertices; }
	inline UINT GetNumVolumeIndices() const				{ return numvolumeindices; }
};

// sample variables
Application*		app					= nullptr;
LPDIRECT3DDEVICE9	device				= nullptr;

LPDIRECT3DTEXTURE9	wood				= nullptr;
LPDIRECT3DTEXTURE9	marble				= nullptr;
LPDIRECT3DTEXTURE9	helptext			= nullptr;

LPD3DXMESH			box					= nullptr;

LPD3DXEFFECT		blinnphong			= nullptr;
LPD3DXEFFECT		simplecolor			= nullptr;
LPD3DXEFFECT		screenquad			= nullptr;

ShadowCaster*		shadowcasters[3]	= { nullptr };
BasicCamera			camera;
BasicCamera			light;
bool				drawvolume			= false;
bool				drawsilhouette		= false;

// --- Sample classes impl ----------------------------------------------------

ShadowCaster::ShadowCaster()
{
	mesh				= nullptr;
	manifold			= nullptr;
	volumevertices		= nullptr;
	volumeindices		= nullptr;
	numvolumevertices	= 0;
	numvolumeindices	= 0;
}

ShadowCaster::~ShadowCaster()
{
	SAFE_RELEASE(volumevertices);
	SAFE_RELEASE(volumeindices);
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(manifold);
}

void ShadowCaster::CreateGeometry(std::function<void (LPD3DXMESH*, LPD3DXMESH*)> callback)
{
	callback(&mesh, &manifold);

	GeometryUtils::PositionVertex* vdata = nullptr;
	uint32_t* idata = nullptr;

	manifold->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&vdata);
	manifold->LockIndexBuffer(D3DLOCK_READONLY, (LPVOID*)&idata);
	{
		GeometryUtils::GenerateEdges(edges, vdata, idata, manifold->GetNumFaces() * 3);
	}
	manifold->UnlockIndexBuffer();
	manifold->UnlockVertexBuffer();

	// worst case scenario
	UINT numverts = 18 * manifold->GetNumFaces();
	UINT numinds = 18 * manifold->GetNumFaces();

	device->CreateVertexBuffer(numverts * sizeof(Math::Vector4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &volumevertices, NULL);
	device->CreateIndexBuffer(numinds * sizeof(uint32_t), D3DUSAGE_DYNAMIC, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &volumeindices, NULL);
}

void ShadowCaster::GenerateSilhouette(const Math::Vector4& position)
{
	Math::Matrix	worldinv;
	Math::Vector3	lp;
	float			dist1, dist2;

	// edges are in object space
	Math::MatrixInverse(worldinv, transform);
	Math::Vec3TransformCoord(lp, (const Math::Vector3&)position, worldinv);

	silhouette.clear();
	silhouette.reserve(50);

	for (size_t i = 0; i < edges.Size(); ++i) {
		const GeometryUtils::Edge& edge = edges[i];

		if (edge.other != UINT_MAX) {
			dist1 = Math::Vec3Dot(lp, edge.n1) - Math::Vec3Dot(edge.v1, edge.n1);
			dist2 = Math::Vec3Dot(lp, edge.n2) - Math::Vec3Dot(edge.v1, edge.n2);

			if ((dist1 < 0) != (dist2 < 0)) {
				// silhouette edge
				if (silhouette.capacity() <= silhouette.size())
					silhouette.reserve(silhouette.size() * 2);

				if (dist1 < 0) {
					std::swap(edge.v1, edge.v2);
					std::swap(edge.n1, edge.n2);
				}

				silhouette.push_back(edge);
			}
		}
	}
}

void ShadowCaster::ExtrudeSilhouette(const Math::Vector4& position)
{
	// find silhouette first
	GenerateSilhouette(position);

	// then extrude
	Math::Vector4*	vdata = nullptr;
	uint32_t*		idata = nullptr;
	Math::Matrix	worldinv;
	Math::Vector3	lp;

	volumevertices->Lock(0, 0, (void**)&vdata, D3DLOCK_DISCARD);
	volumeindices->Lock(0, 0, (void**)&idata, D3DLOCK_DISCARD);

	Math::MatrixInverse(worldinv, transform);
	Math::Vec3TransformCoord(lp, (const Math::Vector3&)position, worldinv);

	// boundary
	for (size_t i = 0; i < silhouette.size(); ++i) {
		const GeometryUtils::Edge& edge = silhouette[i];

		vdata[i * 4 + 0] = Math::Vector4(edge.v1, 1);
		vdata[i * 4 + 1] = Math::Vector4(edge.v1 - lp, 0);
		vdata[i * 4 + 2] = Math::Vector4(edge.v2, 1);
		vdata[i * 4 + 3] = Math::Vector4(edge.v2 - lp, 0);

		idata[i * 6 + 0] = (uint32_t)(i * 4 + 0);
		idata[i * 6 + 1] = (uint32_t)(i * 4 + 1);
		idata[i * 6 + 2] = (uint32_t)(i * 4 + 2);

		idata[i * 6 + 3] = (uint32_t)(i * 4 + 2);
		idata[i * 6 + 4] = (uint32_t)(i * 4 + 1);
		idata[i * 6 + 5] = (uint32_t)(i * 4 + 3);
	}

	numvolumevertices = (UINT)(silhouette.size() * 4);
	numvolumeindices = (UINT)(silhouette.size() * 6);

	// front & back cap
	Math::Vector3 a, b, n;
	GeometryUtils::PositionVertex* mvdata = nullptr;
	uint32_t* midata = nullptr;
	float dist;

	manifold->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&mvdata);
	manifold->LockIndexBuffer(D3DLOCK_READONLY, (LPVOID*)&midata);
	{
		// only add triangles that face the light
		for (uint32_t i = 0; i < manifold->GetNumFaces() * 3; i += 3) {
			uint32_t i1 = midata[i + 0];
			uint32_t i2 = midata[i + 1];
			uint32_t i3 = midata[i + 2];

			const GeometryUtils::PositionVertex& v1 = *(mvdata + i1);
			const GeometryUtils::PositionVertex& v2 = *(mvdata + i2);
			const GeometryUtils::PositionVertex& v3 = *(mvdata + i3);

			Math::Vec3Subtract(a, (const Math::Vector3&)v2, (const Math::Vector3&)v1);
			Math::Vec3Subtract(b, (const Math::Vector3&)v3, (const Math::Vector3&)v1);
			Math::Vec3Cross(n, a, b);

			dist = Math::Vec3Dot(n, lp) - Math::Vec3Dot(n, (const Math::Vector3&)v1);

			if (dist > 0) {
				vdata[numvolumevertices + 0] = Math::Vector4(v1.x, v1.y, v1.z, 1);
				vdata[numvolumevertices + 1] = Math::Vector4(v2.x, v2.y, v2.z, 1);
				vdata[numvolumevertices + 2] = Math::Vector4(v3.x, v3.y, v3.z, 1);

				vdata[numvolumevertices + 3] = Math::Vector4(v1.x - lp.x, v1.y - lp.y, v1.z - lp.z, 0);
				vdata[numvolumevertices + 4] = Math::Vector4(v3.x - lp.x, v3.y - lp.y, v3.z - lp.z, 0);
				vdata[numvolumevertices + 5] = Math::Vector4(v2.x - lp.x, v2.y - lp.y, v2.z - lp.z, 0);
				
				idata[numvolumeindices + 0] = numvolumevertices;
				idata[numvolumeindices + 1] = numvolumevertices + 1;
				idata[numvolumeindices + 2] = numvolumevertices + 2;

				idata[numvolumeindices + 3] = numvolumevertices + 3;
				idata[numvolumeindices + 4] = numvolumevertices + 4;
				idata[numvolumeindices + 5] = numvolumevertices + 5;

				numvolumevertices += 6;
				numvolumeindices += 6;
			}
		}
	}
	manifold->UnlockIndexBuffer();
	manifold->UnlockVertexBuffer();

	volumeindices->Unlock();
	volumevertices->Unlock();
}

void ShadowCaster::SetTransform(const Math::Matrix& transform)
{
	this->transform = transform;
}

// --- Sampple functions impl -------------------------------------------------

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/box.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &box)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/marble.dds", &marble)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/wood2.jpg", &wood)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blinnphong.fx", &blinnphong)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/simplecolor.fx", &simplecolor)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate light\n\n1 - Toggle silhouette\n2 - Toggle shadow volume", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// setup shadow casters
	D3DVERTEXELEMENT9 vertexlayout1[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 vertexlayout2[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	Math::Matrix transform;

	shadowcasters[0] = new ShadowCaster();
	shadowcasters[1] = new ShadowCaster();
	shadowcasters[2] = new ShadowCaster();

	Math::MatrixTranslation(transform, -1.5f, 0.55f, -1.2f);
	shadowcasters[0]->SetTransform(transform);

	Math::MatrixTranslation(transform, 1.0f, 0.55f, -0.7f);
	shadowcasters[1]->SetTransform(transform);

	Math::MatrixTranslation(transform, 0.0f, 0.55f, 1.0f);
	shadowcasters[2]->SetTransform(transform);

	shadowcasters[0]->CreateGeometry([&](LPD3DXMESH* mesh, LPD3DXMESH* manifold) {
		// box
		D3DXCreateMesh(12, 24, D3DXMESH_32BIT|D3DXMESH_MANAGED, vertexlayout1, device, mesh);
		D3DXCreateMesh(12, 8, D3DXMESH_32BIT|D3DXMESH_SYSTEMMEM, vertexlayout2, device, manifold);

		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		(*mesh)->LockVertexBuffer(0, (LPVOID*)&vdata1);
		(*mesh)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::CreateBox(vdata1, idata, 1, 1, 1);
		}
		(*mesh)->UnlockIndexBuffer();
		(*mesh)->UnlockVertexBuffer();

		(*manifold)->LockVertexBuffer(0, (LPVOID*)&vdata2);
		(*manifold)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::Create2MBox(vdata2, idata, (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON), (1 - MANIFOLD_EPSILON));
		}
		(*manifold)->UnlockIndexBuffer();
		(*manifold)->UnlockVertexBuffer();
	});

	shadowcasters[1]->CreateGeometry([&](LPD3DXMESH* mesh, LPD3DXMESH* manifold) {
		// sphere
		uint32_t numverts, numinds, num2mverts, num2minds;

		GeometryUtils::NumVerticesIndicesSphere(numverts, numinds, 32, 32);
		GeometryUtils::NumVerticesIndices2MSphere(num2mverts, num2minds, 32, 32);

		D3DXCreateMesh(numinds / 3, numverts, D3DXMESH_32BIT|D3DXMESH_MANAGED, vertexlayout1, device, mesh);
		D3DXCreateMesh(num2minds / 3, num2mverts, D3DXMESH_32BIT|D3DXMESH_SYSTEMMEM, vertexlayout2, device, manifold);

		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		(*mesh)->LockVertexBuffer(0, (LPVOID*)&vdata1);
		(*mesh)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::CreateSphere(vdata1, idata, 0.5f, 32, 32);
		}
		(*mesh)->UnlockIndexBuffer();
		(*mesh)->UnlockVertexBuffer();

		(*manifold)->LockVertexBuffer(0, (LPVOID*)&vdata2);
		(*manifold)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::Create2MSphere(vdata2, idata, 0.5f - MANIFOLD_EPSILON, 32, 32);
		}
		(*manifold)->UnlockIndexBuffer();
		(*manifold)->UnlockVertexBuffer();
	});

	shadowcasters[2]->CreateGeometry([&](LPD3DXMESH* mesh, LPD3DXMESH* manifold) {
		// L-shape
		D3DXCreateMesh(20, 36, D3DXMESH_32BIT|D3DXMESH_MANAGED, vertexlayout1, device, mesh);
		D3DXCreateMesh(20, 12, D3DXMESH_32BIT|D3DXMESH_SYSTEMMEM, vertexlayout2, device, manifold);

		GeometryUtils::CommonVertex* vdata1 = nullptr;
		GeometryUtils::PositionVertex* vdata2 = nullptr;
		uint32_t* idata = nullptr;

		(*mesh)->LockVertexBuffer(0, (LPVOID*)&vdata1);
		(*mesh)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::CreateLShape(vdata1, idata, 1.5f, 1.0f, 1, 1.2f, 0.6f);
		}
		(*mesh)->UnlockIndexBuffer();
		(*mesh)->UnlockVertexBuffer();

		(*manifold)->LockVertexBuffer(0, (LPVOID*)&vdata2);
		(*manifold)->LockIndexBuffer(0, (LPVOID*)&idata);
		{
			GeometryUtils::Create2MLShape(vdata2, idata, 1.5f - MANIFOLD_EPSILON, 1.0f, 1 - MANIFOLD_EPSILON, 1.2f - MANIFOLD_EPSILON, 0.6f);
		}
		(*manifold)->UnlockIndexBuffer();
		(*manifold)->UnlockVertexBuffer();
	});

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(45), Math::DegreesToRadians(45), 0);

	// setup light
	light.SetDistance(10);
	light.SetPosition(0, 0, 0);
	light.SetOrientation(Math::DegreesToRadians(153), Math::DegreesToRadians(45), 0);
	light.SetPitchLimits(0.1f, 1.45f);

	return true;
}

void UninitScene()
{
	delete shadowcasters[0];
	delete shadowcasters[1];
	delete shadowcasters[2];

	SAFE_RELEASE(wood);
	SAFE_RELEASE(marble);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(box);

	SAFE_RELEASE(blinnphong);
	SAFE_RELEASE(simplecolor);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		drawsilhouette = !drawsilhouette;
		break;

	case KeyCode2:
		drawvolume = !drawvolume;
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		camera.OrbitRight(Math::DegreesToRadians(dx));
		camera.OrbitUp(Math::DegreesToRadians(dy));
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	} else if (state & MouseButtonRight) {
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
	light.Update(dt);
}

void RenderScene(LPD3DXEFFECT effect)
{
	Math::Matrix world, worldinv;
	Math::Vector4 uv(3, 3, 0, 0);

	Math::MatrixScaling(world, 5, 0.1f, 5);
	Math::MatrixInverse(worldinv, world);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	effect->SetMatrix("matWorld", (D3DXMATRIX*)&world);
	effect->SetMatrix("matWorldInv", (D3DXMATRIX*)&worldinv);
	effect->SetVector("uv", (D3DXVECTOR4*)&uv);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		device->SetTexture(0, wood);
		box->DrawSubset(0);

		if (!drawsilhouette) {
			uv.x = uv.y = 1;
			effect->SetVector("uv", (D3DXVECTOR4*)&uv);

			device->SetTexture(0, marble);

			for (int i = 0; i < 3; ++i) {
				world = shadowcasters[i]->GetTransform();
				Math::MatrixInverse(worldinv, world);

				effect->SetMatrix("matWorld", (D3DXMATRIX*)&world);
				effect->SetMatrix("matWorldInv", (D3DXMATRIX*)&worldinv);
				effect->CommitChanges();

				shadowcasters[i]->GetMesh()->DrawSubset(0);
			}
		}
	}
	effect->EndPass();
	effect->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void DrawShadowVolumes()
{
	simplecolor->Begin(NULL, 0);
	simplecolor->BeginPass(0);
	{
		for (int i = 0; i < 3; ++i) {
			ShadowCaster* caster = shadowcasters[i];
			Math::Matrix world = caster->GetTransform();

			simplecolor->SetMatrix("matWorld", (D3DXMATRIX*)&world);
			simplecolor->CommitChanges();

			device->SetStreamSource(0, caster->GetVolumeVertices(), 0, sizeof(Math::Vector4));
			device->SetIndices(caster->GetVolumeIndices());
			device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, caster->GetNumVolumeVertices(), 0, caster->GetNumVolumeIndices() / 3);
		}
	}
	simplecolor->EndPass();
	simplecolor->End();
}

void FillStencilBuffer()
{
	device->SetFVF(D3DFVF_XYZW);
	device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);

	// Carmack's reverse
	device->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS);
	device->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_INCR);

	device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);

	DrawShadowVolumes();

	device->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix	view, proj;
	Math::Matrix	viewproj;
	Math::Vector4	eye;
	Math::Vector4	lightpos(0, 0, 0, 1);

	light.Animate(alpha);
	light.GetEyePosition((Math::Vector3&)lightpos);

	// calculate shadow volumes
	for (int i = 0; i < 3; ++i) {
		shadowcasters[i]->ExtrudeSilhouette(lightpos);
	}

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	// put far plane to infinity
	proj._33 = -1;
	proj._43 = -camera.GetNearPlane();

	Math::MatrixMultiply(viewproj, view, proj);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		D3DXVECTOR4 black(0, 0, 0, 1);
		D3DXVECTOR4 yellow(1, 1, 0, 0.5f);

		device->GetViewport(&oldviewport);

		// z-only pass
		simplecolor->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		simplecolor->SetVector("matColor", &black);

		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		{
			RenderScene(simplecolor);
		}
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		// fill stencil buffer
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		{
			FillStencilBuffer();
		}
		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
		
		// render scene
		blinnphong->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		blinnphong->SetVector("eyePos", (D3DXVECTOR4*)&eye);
		blinnphong->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);
		blinnphong->SetVector("lightAmbient", &black);

		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		device->SetRenderState(D3DRS_STENCILREF, 0);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		{
			RenderScene(blinnphong);
		}
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
		
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

		if (drawsilhouette) {
			device->SetFVF(D3DFVF_XYZW);

			simplecolor->SetVector("matColor", &yellow);
			simplecolor->Begin(0, 0);
			simplecolor->BeginPass(0);
			{
				for (int i = 0; i < 3; ++i) {
					const ShadowCaster* caster = shadowcasters[i];
					const auto& silhouette = caster->GetSilhouette();

					D3DXVECTOR4* verts = (D3DXVECTOR4*)malloc(silhouette.size() * 2 * sizeof(D3DXVECTOR4));
					const Math::Matrix& world = caster->GetTransform();

					for (size_t j = 0; j < silhouette.size(); ++j) {
						const GeometryUtils::Edge& edge = silhouette[j];

						verts[j * 2 + 0] = D3DXVECTOR4(edge.v1.x, edge.v1.y, edge.v1.z, 1);
						verts[j * 2 + 1] = D3DXVECTOR4(edge.v2.x, edge.v2.y, edge.v2.z, 1);
					}

					simplecolor->SetMatrix("matWorld", (D3DXMATRIX*)&world);
					simplecolor->CommitChanges();

					device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)silhouette.size(), verts, sizeof(D3DXVECTOR4));
					free(verts);
				}
			}
			simplecolor->EndPass();
			simplecolor->End();
		}

		if (drawvolume) {
			device->SetFVF(D3DFVF_XYZW);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			simplecolor->SetVector("matColor", &yellow);

			DrawShadowVolumes();

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

		// render text
		viewport = oldviewport;

		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

		device->SetViewport(&viewport);
		device->SetTexture(0, helptext);

		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);

		screenquad->Begin(NULL, 0);
		screenquad->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		screenquad->EndPass();
		screenquad->End();

		// reset states
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetViewport(&oldviewport);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIDirect3D9)) {
		delete app;
		return 1;
	}

	device = (LPDIRECT3DDEVICE9)app->GetLogicalDevice();

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseScrollCallback = MouseScroll;

	app->Run();
	delete app;

	return 0;
}
