
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\pbr.h"
#include "..\Common\fpscamera.h"
#include "..\Common\physicsworld.h"
#include "..\Common\gtaorenderer.h"
#include "..\Common\averageluminance.h"

#ifdef _WIN32
// NOTE: include after gl4ext.h
#	include <gdiplus.h>
#endif

#define OBJECT_ID_ROOM		0
#define OBJECT_ID_SOFA		1
#define OBJECT_ID_BUDDHA1	2
#define OBJECT_ID_BUDDHA2	3
#define OBJECT_ID_BUDDHA3	4
#define OBJECT_ID_BUDDHA4	5
#define OBJECT_ID_CAR		6
#define OBJECT_ID_TABLE		7
#define OBJECT_ID_TV		8
#define OBJECT_ID_DRAGON1	9
#define OBJECT_ID_DRAGON2	10
#define OBJECT_ID_DRAGON3	11
#define OBJECT_ID_DRAGON4	12
#define OBJECT_ID_CUPBOARDS	13
#define OBJECT_ID_ROOFTOP	14
#define OBJECT_ID_LAMP		15

#define LIGHT_ID_SKY		0
#define LIGHT_ID_LOCAL1		1
#define LIGHT_ID_SPOT		2
#define LIGHT_ID_TUBE		3
#define LIGHT_ID_RECT		4

#define NUM_OBJECTS			16
#define NUM_LIGHTS			5
#define SHADOWMAP_SIZE		1024
#define USE_MSAA			0 //1

// helper macros
#define TITLE				"Shader sample 53: Physically Based Rendering"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample enums
enum LightMode
{
	LightModeAll = 0,
	LightModeAllExceptSky,
	LightModeSkyOnly
};

// sample classes
class SceneObject
{
	typedef std::vector<const PBRMaterial*> MaterialList;

private:
	MaterialList	materials;
	Math::AABox		boundingbox;
	RigidBody*		body;
	OpenGLMesh*		mesh;
	GLuint			numsubsets;
	float			scaling;
	bool			isclone;
	bool			isvisible;	// for view frustum culling

public:
	SceneObject(const SceneObject& other);
	SceneObject(const char* file, RigidBodyType type, float scale = 1.0f);
	SceneObject(OpenGLMesh* external, RigidBodyType type, float scale = 1.0f);
	~SceneObject();

	void RecalculateBoundingBox();
	void Draw(OpenGLEffect* effect, bool transparent);
	void DrawFast(OpenGLEffect* effect);

	inline void SetMaterial(size_t index, const PBRMaterial* mat)	{ materials[index] = mat; }
	inline void SetVisible(bool visible)							{ isvisible = visible; }

	inline const Math::AABox& GetBoundingBox() const				{ return boundingbox; }
	inline const PBRMaterial* GetMaterial(size_t index) const		{ return materials[index]; }
	inline OpenGLMesh* GetMesh()									{ return mesh; }
	inline RigidBody* GetBody()										{ return body; }
};

// sample variables
Application*		app						= nullptr;

OpenGLMesh*			debugbox				= nullptr;
OpenGLMesh*			skymesh					= nullptr;
OpenGLScreenQuad*	screenquad				= nullptr;

OpenGLFramebuffer*	gbuffer					= nullptr;
OpenGLFramebuffer*	msaaframebuffer			= nullptr;
OpenGLFramebuffer*	framebuffer1			= nullptr;
OpenGLFramebuffer*	framebuffer2			= nullptr;

OpenGLEffect*		simplecolor				= nullptr;
OpenGLEffect*		skyeffect				= nullptr;
OpenGLEffect*		gbuffereffect			= nullptr;
OpenGLEffect*		pbrlightprobe			= nullptr;
OpenGLEffect*		pbrspotlight			= nullptr;
OpenGLEffect*		pbrarealight			= nullptr;
OpenGLEffect*		gtaocombine				= nullptr;
OpenGLEffect*		tonemap					= nullptr;

GLuint				environment				= 0;
GLuint				integratedbrdf			= 0;
GLuint				infotex					= 0;
GLuint				helptext				= 0;

Math::AABox			scenebox;
PBRMaterialTable	materials;
SceneObject*		objects[NUM_OBJECTS]	= { nullptr };
PBRLight*			lights[NUM_LIGHTS]		= { nullptr };
GTAORenderer*		gtaorenderer			= nullptr;
AverageLuminance*	averageluminance		= nullptr;
PhysicsWorld*		physicsworld			= nullptr;
RigidBody*			selectedbody			= nullptr;
FPSCamera*			camera					= nullptr;
bool				debugphysics			= false;
bool				drawtext				= true;
int					gtaomode				= 2;

void ResetCamera();

void DEBUG_DrawBody(RigidBodyType type, const Math::Matrix& xform);

// static functions
static bool IsAffectedByLight(int object, int light)
{
	bool affected = false;

	if (light == LIGHT_ID_SKY) {
		affected |=
			(object == OBJECT_ID_ROOM || object == OBJECT_ID_ROOFTOP ||
			(object > OBJECT_ID_SOFA && object < OBJECT_ID_TABLE));
	}

	if (light == LIGHT_ID_LOCAL1) {
		affected |= (
			object == OBJECT_ID_LAMP ||
			object == OBJECT_ID_CUPBOARDS ||
			object == OBJECT_ID_TABLE ||
			object == OBJECT_ID_DRAGON3 ||
			object == OBJECT_ID_TV);
	}

	if (light == LIGHT_ID_SPOT)
		affected |= (object < OBJECT_ID_ROOFTOP);

	if (light == LIGHT_ID_TUBE)
		affected = (object == OBJECT_ID_ROOFTOP);

	if (light == LIGHT_ID_RECT)
		affected = (object <= OBJECT_ID_SOFA || (object >= OBJECT_ID_DRAGON1 && object <= OBJECT_ID_DRAGON4));

	return affected;
}

static Math::AABox CalculateShadowBox(int light)
{
	Math::AABox shadowbox;

	for (int j = 0; j < NUM_OBJECTS; ++j) {
		if (objects[j] != nullptr && IsAffectedByLight(j, light)) {
			const Math::AABox& box = objects[j]->GetMesh()->GetBoundingBox();

			shadowbox.Add(box.Min);
			shadowbox.Add(box.Max);
		}
	}

	return shadowbox;
}

static void UpdateMaterialInfo(const PBRMaterial& material)
{
#ifdef _WIN32
	Gdiplus::PrivateFontCollection	privfonts;
	Gdiplus::SolidBrush				brush1(Gdiplus::Color(0x96000000));
	Gdiplus::SolidBrush				brush2(Gdiplus::Color(0xaa000000));
	Gdiplus::SolidBrush				brush3(Gdiplus::Color(0x8c000000));
	Gdiplus::SolidBrush				brush4(Gdiplus::Color(0xffeeeeee));
	Gdiplus::SolidBrush				brush5(Gdiplus::Color(0xffaaaaaa));

	Gdiplus::FontFamily				family;
	Gdiplus::StringFormat			format;
	Gdiplus::GraphicsPath			path1;
	Gdiplus::GraphicsPath			path2;
	Gdiplus::Color					textfill(0xffffffff);

	Gdiplus::Bitmap*				bitmap;
	Gdiplus::Graphics*				graphics;

	std::wstring					wstr;
	INT								found;
	int								length;
	int								start = 256 - 212;

	privfonts.AddFontFile(L"../../Media/Fonts/AGENCYR.ttf");
	privfonts.GetFamilies(1, &family, &found);

	bitmap = new Gdiplus::Bitmap(512, 256, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	// render info
	graphics->FillRectangle(&brush1, 0, start, 512, 212);
	graphics->FillRectangle(&brush2, 10, start + 10, 492, 48);
	graphics->FillRectangle(&brush3, 10, start + 68, 492, 134);

	path1.AddString(material.Name.c_str(), (INT)material.Name.length(), &family, 0, 44, Gdiplus::Point(10, start + 10), &format);

	path2.AddString(L"Base color", 10, &family, 0, 32, Gdiplus::Point(12, start + 68 + 5), &format);
	path2.AddString(L"Roughness", 9, &family, 0, 32, Gdiplus::Point(12, start + 112 + 5), &format);
	path2.AddString(L"Metalness", 9, &family, 0, 32, Gdiplus::Point(12, start + 156 + 5), &format);

	format.SetAlignment(Gdiplus::StringAlignmentFar);
	wstr.resize(32);

	if (material.Texture == 0) {
		length = swprintf(&wstr[0], 32, L"(%.3f, %.3f, %.3f)", material.BaseColor.r, material.BaseColor.g, material.BaseColor.b);
	} else {
		wstr = material.GetTextureName();
		length = (int)wstr.length();
	}
	
	path1.AddString(wstr.c_str(), length, &family, 0, 32, Gdiplus::Point(480, start + 68 + 5), &format);

	length = swprintf(&wstr[0], 32, L"%.2f", material.Roughness);
	path1.AddString(wstr.c_str(), length, &family, 0, 32, Gdiplus::Point(480, start + 112 + 5), &format);

	length = swprintf(&wstr[0], 32, L"%.2f", material.Metalness);
	path1.AddString(wstr.c_str(), length, &family, 0, 32, Gdiplus::Point(480, start + 156 + 5), &format);

	graphics->FillPath(&brush4, &path1);
	graphics->FillPath(&brush5, &path2);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));
	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

	glBindTexture(GL_TEXTURE_2D, infotex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data.Scan0);

	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
#else
	// TODO:
#endif
}

// --- SceneObject impl -------------------------------------------------------

SceneObject::SceneObject(const SceneObject& other)
{
	if (other.mesh != nullptr && other.body != nullptr ) {
		Math::Vector3 size, center;
		const Math::AABox& box = other.mesh->GetBoundingBox();

		if (other.body->GetType() == RigidBodyTypeBox) {
			box.GetSize(size);
			box.GetCenter(center);

			body = physicsworld->AddStaticBox(size[0] * other.scaling, size[1] * other.scaling, size[2] * other.scaling);

			Math::Vec3Scale(center, center, -other.scaling);
			body->SetPivot(center);
		} else {
			body = nullptr;
		}
	}

	mesh		= other.mesh;
	numsubsets	= other.numsubsets;
	scaling		= other.scaling;
	isclone		= true;
	isvisible	= true;

	materials.resize(numsubsets);
	RecalculateBoundingBox();
}

SceneObject::SceneObject(const char* file, RigidBodyType type, float scale)
{
	bool success = GLCreateMeshFromQM(file, &mesh);

	if (success) {
		Math::Vector3 size, center;
		const Math::AABox& box = mesh->GetBoundingBox();

		if (type == RigidBodyTypeBox) {
			box.GetSize(size);
			box.GetCenter(center);

			body = physicsworld->AddStaticBox(size[0] * scale, size[1] * scale, size[2] * scale);

			Math::Vec3Scale(center, center, -scale);
			body->SetPivot(center);
		} else {
			body = nullptr;
		}

		scaling = scale;
		isclone = false;

		numsubsets = mesh->GetNumSubsets();

		materials.resize(numsubsets);
		RecalculateBoundingBox();
	}

	isvisible = true;
}

SceneObject::SceneObject(OpenGLMesh* external, RigidBodyType type, float scale)
{
	if (external != nullptr) {
		Math::Vector3 size, center;
		const Math::AABox& box = external->GetBoundingBox();

		if (type == RigidBodyTypeBox) {
			box.GetSize(size);
			box.GetCenter(center);

			body = physicsworld->AddStaticBox(size[0] * scale, size[1] * scale, size[2] * scale);

			Math::Vec3Scale(center, center, -scale);
			body->SetPivot(center);
		} else {
			body = nullptr;
		}

		mesh		= external;
		numsubsets	= external->GetNumSubsets();
		scaling		= scale;
		isclone		= false;

		materials.resize(numsubsets);
		RecalculateBoundingBox();
	}

	isvisible = true;
}

SceneObject::~SceneObject()
{
	if (!isclone)
		delete mesh;
}

void SceneObject::RecalculateBoundingBox()
{
	Math::Matrix world;

	if (body != nullptr) {
		body->GetTransformWithSize(world);

		boundingbox.Min = Math::Vector3(-0.5f, -0.5f, -0.5f);
		boundingbox.Max = Math::Vector3(0.5f, 0.5f, 0.5f);

		boundingbox.TransformAxisAligned(world);
	} else if (mesh != nullptr) {
		boundingbox = mesh->GetBoundingBox();

		Math::Vec3Scale(boundingbox.Min, boundingbox.Min, scaling);
		Math::Vec3Scale(boundingbox.Max, boundingbox.Max, scaling);
	}
}

void SceneObject::Draw(OpenGLEffect* effect, bool transparent)
{
	if (mesh == nullptr || !isvisible)
		return;

	Math::Matrix world, worldinv;

	Math::MatrixScaling(world, scaling, scaling, scaling);
	Math::MatrixScaling(worldinv, 1.0f / scaling, 1.0f / scaling, 1.0f / scaling);

	if (body != nullptr) {
		Math::MatrixMultiply(world, world, body->GetTransform());
		Math::MatrixMultiply(worldinv, body->GetInverseTransform(), worldinv);
	}

	effect->SetMatrix("matWorld", world);
	effect->SetMatrix("matWorldInv", worldinv);

	for (GLuint i = 0; i < numsubsets; ++i) {
		const PBRMaterial* mat = materials[i];
		float matparams[] = { mat->Roughness, mat->Metalness, 0, 0 };

		if (mat->Transparent != transparent)
			continue;

		if (mat->Texture != 0) {
			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, mat->Texture);
			matparams[2] = 1;
		}

		effect->SetVector("baseColor", &mat->BaseColor.r);
		effect->SetVector("matParams", matparams);
		effect->CommitChanges();

		mesh->DrawSubset(i);
	}
}

void SceneObject::DrawFast(OpenGLEffect* effect)
{
	if (mesh == nullptr || !isvisible )
		return;

	Math::Matrix world;
	Math::MatrixScaling(world, scaling, scaling, scaling);

	if (body != nullptr)
		Math::MatrixMultiply(world, world, body->GetTransform());

	effect->SetMatrix("matWorld", world);
	effect->CommitChanges();

	for (GLuint i = 0; i < numsubsets; ++i) {
		if (!materials[i]->Transparent)
			mesh->DrawSubset(i);
	}
}

// --- Sample impl ------------------------------------------------------------

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	physicsworld = new PhysicsWorld();

	// setup material library
	materials.insert({ "Asphalt", new PBRMaterial(L"Asphalt", Math::Color(0.1f, 0.1f, 0.1f, 1), 0.8f, 0, "../../Media/Textures/asphalt2.jpg") });
	materials.insert({ "Carpet", new PBRMaterial(L"Carpet", Math::Color(0.1f, 0.1f, 0.1f, 1), 0.8f, 0, "../../Media/Textures/carpet4.jpg") });
	materials.insert({ "Stucco", new PBRMaterial(L"Stucco", Math::Color(1, 1, 1, 1), 1.0f, 0, "../../Media/Textures/stucco.jpg") });
	materials.insert({ "Tapestry", new PBRMaterial(L"Tapestry", Math::Color::sRGBToLinear(175, 149, 151), 0.8f, 0, nullptr) });
	materials.insert({ "Window", new PBRMaterial(L"Window glass", Math::Color(0.03f, 0.03f, 0.03f, 0.5f), 0, 0, nullptr, true) });
	materials.insert({ "Black", new PBRMaterial(L"Black", Math::Color(0.01f, 0.01f, 0.01f, 1.0f), 0.15f, 0, nullptr) });
	materials.insert({ "Wood", new PBRMaterial(L"Wood", Math::Color::sRGBToLinear(119, 77, 0), 0.1f, 0, "../../Media/Textures/wood1.jpg") });
	materials.insert({ "Aluminium", new PBRMaterial(L"Aluminium", Math::Color(0.913f, 0.922f, 0.924f, 1.0f), 0.5f, 1, nullptr) });
	materials.insert({ "White", new PBRMaterial(L"White wall", Math::Color(1, 1, 1, 1), 1.0f, 0, nullptr) });

	materials.insert({ "BlackPlastic", new PBRMaterial(L"Black plastic", Math::Color(0, 0, 0, 1), 0.1f, 0, nullptr) });
	materials.insert({ "BlackLeather", new PBRMaterial(L"Black leather", Math::Color(6.2e-3f, 5.14e-3f, 7.08e-3f, 1), 0.55f, 0, nullptr) });

	materials.insert({ "DarkGreyPlastic", new PBRMaterial(L"Dark grey plastic", Math::Color(6.2e-3f, 6.2e-3f, 6.2e-3f, 1), 0.55f, 0, nullptr) });
	materials.insert({ "WhitePlastic", new PBRMaterial(L"White plastic", Math::Color(1, 1, 1, 1), 0.05f, 0, nullptr) });

	materials.insert({ "TV", new PBRMaterial(L"TV", Math::Color(0, 0, 0, 1), 0, 0, "../../Media/Textures/scarlett.jpg") });
	
	materials.insert({ "BrownCupboard", new PBRMaterial(L"Brown cupboard", Math::Color::sRGBToLinear(190, 174, 149), 0, 0, nullptr) });
	materials.insert({ "BlackCupboard", new PBRMaterial(L"Black cupboard", Math::Color(0, 0, 0, 1), 0, 0, nullptr) });
	materials.insert({ "WhiteCupboard", new PBRMaterial(L"White cupboard", Math::Color(1, 1, 1, 1), 0, 0, nullptr) });
	materials.insert({ "WhiteBoard", new PBRMaterial(L"White board", Math::Color(1, 1, 1, 1), 1, 0, nullptr) });

	materials.insert({ "BlueRubber", new PBRMaterial(L"Blue rubber (MERL)", Math::Color(0.05f, 0.08f, 0.17f, 1), 0.65f, 0, nullptr) });
	materials.insert({ "RedPlastic", new PBRMaterial(L"Red specular plastic (MERL)", Math::Color(0.26f, 0.05f, 0.01f, 1), 0.08f, 0, nullptr) });
	materials.insert({ "YellowPaint", new PBRMaterial(L"Yellow paint (MERL)", Math::Color(0.32f, 0.22f, 0.05f, 1), 0.68f, 0, nullptr) });

	materials.insert({ "Paint1", new PBRMaterial(L"Car paint", Math::Color(1.0f, 0.2f, 0.0f, 1), 0.1f, 0.5f, nullptr) });
	materials.insert({ "Paint2", new PBRMaterial(L"Car paint black", Math::Color(0.0f, 0.0f, 0.0f, 1), 0.1f, 0.5f, nullptr) });
	materials.insert({ "Chromium", new PBRMaterial(L"Chromium", Math::Color(0.549f, 0.556f, 0.554f, 1), 0.2f, 1, nullptr) });
	materials.insert({ "Glass", new PBRMaterial(L"Glass", Math::Color(0.03f, 0.03f, 0.03f, 0.25f), 0.05f, 0, nullptr, true) });
	materials.insert({ "BlackPolymer", new PBRMaterial(L"Black polymer", Math::Color(0, 0, 0, 1), 0.5f, 0, nullptr) });
	materials.insert({ "GreyPolymer", new PBRMaterial(L"Grey polymer", Math::Color(0.01f, 0.01f, 0.01f, 1), 1.0f, 0, nullptr) });
	materials.insert({ "RedLeather", new PBRMaterial(L"Red leather", Math::Color(0.05f, 5.14e-4f, 7.08e-4f, 1), 0.9f, 0, nullptr) });
	materials.insert({ "BlackRubber", new PBRMaterial(L"Black rubber", Math::Color(6.2e-4f, 5.14e-4f, 7.08e-4f, 1), 0.65f, 0, nullptr) });
	materials.insert({ "Mirror", new PBRMaterial(L"Mirror", Math::Color(0.972f, 0.96f, 0.915f, 1), 0, 1, nullptr) });
	
	materials.insert({ "Gold", new PBRMaterial(L"Gold", Math::Color(1.022f, 0.782f, 0.344f, 1), 0.1f, 1, nullptr) });
	materials.insert({ "Silver", new PBRMaterial(L"Silver", Math::Color(0.972f, 0.96f, 0.915f, 1), 0.1f, 1, nullptr) });
	materials.insert({ "Copper", new PBRMaterial(L"Copper", Math::Color(0.955f, 0.638f, 0.538f, 1), 0.1f, 1, nullptr) });
	materials.insert({ "Zinc", new PBRMaterial(L"Zinc", Math::Color(0.664f, 0.824f, 0.85f, 1), 0.1f, 1, nullptr) });
	
	materials.insert({ "LampOutside", new PBRMaterial(L"Lamp outside", Math::Color(0, 0, 0, 1), 0.5f, 0, nullptr) });
	materials.insert({ "LampInside", new PBRMaterial(L"Lamp inside", Math::Color(10, 10, 10, 1), 0.2f, 0, nullptr) });
	materials.insert({ "LampInsideOff", new PBRMaterial(L"Lamp inside", Math::Color(0.5f, 0.5f, 0.5f, 1), 0.2f, 0, nullptr) });

	// load scene
	Math::Quaternion orientation;

	objects[OBJECT_ID_ROOM] = new SceneObject("../../Media/MeshesQM/livingroom.qm", RigidBodyTypeNone);

	objects[OBJECT_ID_ROOM]->SetMaterial(0, materials["Stucco"]);
	objects[OBJECT_ID_ROOM]->SetMaterial(1, materials["Tapestry"]);
	objects[OBJECT_ID_ROOM]->SetMaterial(2, materials["Window"]);
	objects[OBJECT_ID_ROOM]->SetMaterial(3, materials["Black"]);		// stair rail
	objects[OBJECT_ID_ROOM]->SetMaterial(4, materials["Wood"]);			// stair
	objects[OBJECT_ID_ROOM]->SetMaterial(5, materials["Asphalt"]);
	objects[OBJECT_ID_ROOM]->SetMaterial(6, materials["Carpet"]);
	objects[OBJECT_ID_ROOM]->SetMaterial(7, materials["Aluminium"]);	// step
	objects[OBJECT_ID_ROOM]->SetMaterial(8, materials["White"]);		// shelve

	objects[OBJECT_ID_ROOFTOP] = new SceneObject("../../Media/MeshesQM/rooftop.qm", RigidBodyTypeNone);

	objects[OBJECT_ID_ROOFTOP]->SetMaterial(0, materials["Carpet"]);
	objects[OBJECT_ID_ROOFTOP]->SetMaterial(1, materials["White"]);

	// sofa
	objects[OBJECT_ID_SOFA] = new SceneObject("../../Media/MeshesQM/sofa2.qm", RigidBodyTypeBox);

	objects[OBJECT_ID_SOFA]->SetMaterial(0, materials["BlackLeather"]);
	objects[OBJECT_ID_SOFA]->SetMaterial(1, materials["BlackPlastic"]);

	Math::QuaternionRotationAxis(orientation, Math::HALF_PI, 0, 1, 0);

	objects[OBJECT_ID_SOFA]->GetBody()->SetPosition(-1.65f, 0, -2.0f);
	objects[OBJECT_ID_SOFA]->GetBody()->SetOrientation(orientation);

	// table
	objects[OBJECT_ID_TABLE] = new SceneObject("../../Media/MeshesQM/table.qm", RigidBodyTypeBox);

	GLuint tablesubsets[] = { 5, 1, 2, 3, 4, 0 };
	objects[OBJECT_ID_TABLE]->GetMesh()->ReorderSubsets(tablesubsets);

	objects[OBJECT_ID_TABLE]->SetMaterial(0, materials["WhitePlastic"]);	// upper table
	objects[OBJECT_ID_TABLE]->SetMaterial(1, materials["WhitePlastic"]);	// leg1
	objects[OBJECT_ID_TABLE]->SetMaterial(2, materials["BlackPlastic"]);	// plate2
	objects[OBJECT_ID_TABLE]->SetMaterial(3, materials["DarkGreyPlastic"]);	// lower table
	objects[OBJECT_ID_TABLE]->SetMaterial(4, materials["WhitePlastic"]);	// leg2
	objects[OBJECT_ID_TABLE]->SetMaterial(5, materials["BlackPlastic"]);	// plate1

	Math::QuaternionRotationAxis(orientation, Math::HALF_PI, 0, 1, 0);

	objects[OBJECT_ID_TABLE]->GetBody()->SetPosition(-2.7f, 0, -1.25f);
	objects[OBJECT_ID_TABLE]->GetBody()->SetOrientation(orientation);

	// tv
	objects[OBJECT_ID_TV] = new SceneObject("../../Media/MeshesQM/plasmatv.qm", RigidBodyTypeBox);

	objects[OBJECT_ID_TV]->SetMaterial(0, materials["TV"]);
	objects[OBJECT_ID_TV]->SetMaterial(1, materials["DarkGreyPlastic"]);

	Math::QuaternionRotationAxis(orientation, -Math::HALF_PI, 0, 1, 0);

	objects[OBJECT_ID_TV]->GetBody()->SetPosition(-3.87f, 1.4f, -2.15f);
	objects[OBJECT_ID_TV]->GetBody()->SetOrientation(orientation);

	// cupboards
	objects[OBJECT_ID_CUPBOARDS] = new SceneObject("../../Media/MeshesQM/cupboards.qm", RigidBodyTypeBox);

	objects[OBJECT_ID_CUPBOARDS]->SetMaterial(0, materials["BlackCupboard"]);	// black parts
	objects[OBJECT_ID_CUPBOARDS]->SetMaterial(1, materials["WhiteBoard"]);		// board
	objects[OBJECT_ID_CUPBOARDS]->SetMaterial(2, materials["BrownCupboard"]);	// brown parts
	objects[OBJECT_ID_CUPBOARDS]->SetMaterial(3, materials["WhiteCupboard"]);	// white parts

	// dragons
	objects[OBJECT_ID_DRAGON1] = new SceneObject("../../Media/MeshesQM/dragon.qm", RigidBodyTypeBox, 0.05f);
	objects[OBJECT_ID_DRAGON2] = new SceneObject(*objects[OBJECT_ID_DRAGON1]);
	objects[OBJECT_ID_DRAGON3] = new SceneObject(*objects[OBJECT_ID_DRAGON1]);
	objects[OBJECT_ID_DRAGON4] = new SceneObject(*objects[OBJECT_ID_DRAGON1]);

	objects[OBJECT_ID_DRAGON1]->SetMaterial(0, materials["BlueRubber"]);
	objects[OBJECT_ID_DRAGON2]->SetMaterial(0, materials["RedPlastic"]);
	objects[OBJECT_ID_DRAGON3]->SetMaterial(0, materials["Chromium"]);
	objects[OBJECT_ID_DRAGON4]->SetMaterial(0, materials["YellowPaint"]);
	
	Math::QuaternionRotationAxis(orientation, Math::PI, 0, 1, 0);

	objects[OBJECT_ID_DRAGON1]->GetBody()->SetPosition(-3.5f, 1.01f, -4.81f);
	objects[OBJECT_ID_DRAGON1]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_DRAGON2]->GetBody()->SetPosition(-2.5f, 1.01f, -4.81f);
	objects[OBJECT_ID_DRAGON2]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_DRAGON3]->GetBody()->SetPosition(-1.5f, 1.01f, -4.81f);
	objects[OBJECT_ID_DRAGON3]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_DRAGON4]->GetBody()->SetPosition(-0.5f, 1.01f, -4.81f);
	objects[OBJECT_ID_DRAGON4]->GetBody()->SetOrientation(orientation);

	// car
	objects[OBJECT_ID_CAR] = new SceneObject("../../Media/MeshesQM/zonda.qm", RigidBodyTypeBox);

	GLuint carsubsets[] = { 0, 1, 5, 8, 3, 6, 9, 7, 4, 2 };
	objects[OBJECT_ID_CAR]->GetMesh()->ReorderSubsets(carsubsets);

	objects[OBJECT_ID_CAR]->SetMaterial(0, materials["Paint1"]);		// chassis
	objects[OBJECT_ID_CAR]->SetMaterial(1, materials["Paint2"]);		// lower chassis
	objects[OBJECT_ID_CAR]->SetMaterial(2, materials["RedLeather"]);	// interior
	objects[OBJECT_ID_CAR]->SetMaterial(3, materials["BlackPolymer"]);	// black parts
	objects[OBJECT_ID_CAR]->SetMaterial(4, materials["GreyPolymer"]);	// grey parts
	objects[OBJECT_ID_CAR]->SetMaterial(5, materials["Chromium"]);		// wheels
	objects[OBJECT_ID_CAR]->SetMaterial(6, materials["BlackRubber"]);	// tires
	objects[OBJECT_ID_CAR]->SetMaterial(7, materials["Chromium"]);		// exhaust
	objects[OBJECT_ID_CAR]->SetMaterial(8, materials["Mirror"]);		// mirrors
	objects[OBJECT_ID_CAR]->SetMaterial(9, materials["Glass"]);			// windows

	Math::QuaternionRotationAxis(orientation, -Math::PI / 6, 0, 1, 0);

	objects[OBJECT_ID_CAR]->GetBody()->SetPosition(1, 0, 4.5f);
	objects[OBJECT_ID_CAR]->GetBody()->SetOrientation(orientation);

	// buddhas
	objects[OBJECT_ID_BUDDHA1] = new SceneObject("../../Media/MeshesQM/happy1.qm", RigidBodyTypeBox, 10);
	objects[OBJECT_ID_BUDDHA2] = new SceneObject(*objects[OBJECT_ID_BUDDHA1]);
	objects[OBJECT_ID_BUDDHA3] = new SceneObject(*objects[OBJECT_ID_BUDDHA1]);
	objects[OBJECT_ID_BUDDHA4] = new SceneObject(*objects[OBJECT_ID_BUDDHA1]);

	objects[OBJECT_ID_BUDDHA1]->SetMaterial(0, materials["Gold"]);
	objects[OBJECT_ID_BUDDHA2]->SetMaterial(0, materials["Silver"]);
	objects[OBJECT_ID_BUDDHA3]->SetMaterial(0, materials["Copper"]);
	objects[OBJECT_ID_BUDDHA4]->SetMaterial(0, materials["Zinc"]);

	Math::QuaternionRotationAxis(orientation, Math::HALF_PI, 0, 1, 0);

	objects[OBJECT_ID_BUDDHA1]->GetBody()->SetPosition(5.0f, -0.5f, -0.75f);
	objects[OBJECT_ID_BUDDHA1]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_BUDDHA2]->GetBody()->SetPosition(5.0f, -0.5f, 0.25f);
	objects[OBJECT_ID_BUDDHA2]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_BUDDHA3]->GetBody()->SetPosition(5.0f, -0.5f, 1.25f);
	objects[OBJECT_ID_BUDDHA3]->GetBody()->SetOrientation(orientation);

	objects[OBJECT_ID_BUDDHA4]->GetBody()->SetPosition(5.0f, -0.5f, 2.25f);
	objects[OBJECT_ID_BUDDHA4]->GetBody()->SetOrientation(orientation);

	// lamp
	objects[OBJECT_ID_LAMP] = new SceneObject("../../Media/MeshesQM/lamp.qm", RigidBodyTypeNone);

	objects[OBJECT_ID_LAMP]->SetMaterial(0, materials["LampOutside"]);
	objects[OBJECT_ID_LAMP]->SetMaterial(1, materials["LampInside"]);

	// setup physics (room)
	RigidBody* body = 0;

	body = physicsworld->AddStaticBox(14.0f, 0.01f, 16.0f);
	body->SetUserData(MAKE_DWORD_PTR(0, 6));

	body = physicsworld->AddStaticBox(6.0f, 3.1f, 0.2f);
	body->SetPosition(-1.0f, 1.55f, -5.0f);
	body->SetUserData(MAKE_DWORD_PTR(0, 1));

	body = physicsworld->AddStaticBox(0.2f, 3.1f, 6.0f);
	body->SetPosition(-4.0f, 1.55f, -2.0f);
	body->SetUserData(MAKE_DWORD_PTR(0, 1));

	body = physicsworld->AddStaticBox(3.0f, 3.1f, 0.1f);
	body->SetPosition(-2.5f, 1.55f, 1.0f);
	body->SetUserData(MAKE_DWORD_PTR(0, 2));

	body = physicsworld->AddStaticBox(0.2f, 3.1f, 3.0f);
	body->SetPosition(2.0f, 1.55f, -3.5f);
	body->SetUserData(MAKE_DWORD_PTR(0, 1));

	body = physicsworld->AddStaticBox(3.0f, 0.1f, 6.0f);
	body->SetPosition(-2.5f, 3.15f, -2.0f);
	body->SetUserData(MAKE_DWORD_PTR(0, 0));

	body = physicsworld->AddStaticBox(3.0f, 0.1f, 3.0f);
	body->SetPosition(0.5f, 3.15f, -3.5f);
	body->SetUserData(MAKE_DWORD_PTR(0, 0));

	body = physicsworld->AddStaticBox(6.0f, 1.0f, 0.3f);
	body->SetPosition(-1, 0.5f, -4.81f);
	body->SetUserData(MAKE_DWORD_PTR(0, 0));

	// NOTE: upper pad at the end of stairs
	const float padlength		= 2.35f;
	const float padz			= -3.825f;
	const float padtop			= 3.2f;
	const float slopethickness	= 0.1f;

	body = physicsworld->AddStaticBox(1.3f, 0.1f, padlength);
	body->SetPosition(-4.65f, 3.15f, padz);
	body->SetUserData(MAKE_DWORD_PTR(0, 0));

	// NOTE: stair parameters
	float smalldiag		= sqrtf(slopethickness * slopethickness * 0.5f);
	float slopelength	= sqrtf(2 * padtop * padtop);
	float slopey		= ((slopelength - slopethickness) * padtop * 0.5f) / slopelength;
	float slopez		= padz + padlength * 0.5f + (padtop - slopey) - smalldiag;

	body = physicsworld->AddStaticBox(1.0f, slopelength, slopethickness);
	Math::QuaternionRotationAxis(orientation, Math::PI / -4, 1, 0, 0);

	body->SetPosition(-4.62f, slopey, slopez);
	body->SetOrientation(orientation);
	body->SetUserData(MAKE_DWORD_PTR(0, 4));

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/simplecolor.vert", 0, 0, 0, "../../Media/ShadersGL/simplecolor.frag", &simplecolor)) {
		MYERROR("Could not load 'simplecolor' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/sky.vert", 0, 0, 0, "../../Media/ShadersGL/sky.frag", &skyeffect)) {
		MYERROR("Could not load 'sky' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/gbuffer.vert", 0, 0, 0, "../../Media/ShadersGL/gbuffer.frag", &gbuffereffect)) {
		MYERROR("Could not load 'gbuffer' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/pbr_lightprobe.vert", 0, 0, 0, "../../Media/ShadersGL/pbr_lightprobe.frag", &pbrlightprobe)) {
		MYERROR("Could not load 'pbr_lightprobe' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/pbr_spotlight.vert", 0, 0, 0, "../../Media/ShadersGL/pbr_spotlight.frag", &pbrspotlight)) {
		MYERROR("Could not load 'pbr_spotlight' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/pbr_arealight.vert", 0, 0, 0, "../../Media/ShadersGL/pbr_arealight.frag", &pbrarealight)) {
		MYERROR("Could not load 'pbr_arealight' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtaocombine.frag", &gtaocombine)) {
		MYERROR("Could not load 'gtaocombine' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/tonemap.frag", &tonemap)) {
		MYERROR("Could not load 'tonemap' shader");
		return false;
	}

	// TODO: other effects

	Math::Matrix identity(1, 1, 1, 1);
	Math::Vector4 white(1, 1, 1, 1);

	simplecolor->SetVector("matColor", white);
	skyeffect->SetInt("sampler0", 0);

	gbuffereffect->SetInt("sampler0", 0);
	gbuffereffect->SetInt("sampler1", 1);

	pbrlightprobe->SetInt("baseColorSamp", 0);
	pbrlightprobe->SetInt("illumDiffuse", 1);
	pbrlightprobe->SetInt("illumSpecular", 2);
	pbrlightprobe->SetInt("brdfLUT", 3);

	pbrspotlight->SetInt("shadowMap", 2);
	pbrspotlight->SetInt("isPerspective", 1);

	gtaocombine->SetMatrix("matTexture", identity);
	gtaocombine->SetInt("sampler0", 0);
	gtaocombine->SetInt("sampler1", 1);
	gtaocombine->SetInt("renderMode", gtaomode);

	tonemap->SetMatrix("matTexture", identity);
	tonemap->SetInt("renderedScene", 0);
	tonemap->SetInt("averageLuminance", 1);

	// other things
	if (!GLCreateMeshFromQM("../../Media/MeshesQM/sky.qm", &skymesh)) {
		MYERROR("Could not load 'sky' mesh");
		return false;
	}

	GLCreateCubeTextureFromDDS("../../Media/Textures/uffizi.dds", false, &environment);
	GLCreateTextureFromFile("../../Media/Textures/brdf.dds", false, &integratedbrdf);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gtaorenderer = new GTAORenderer(screenwidth, screenheight);
	averageluminance = new AverageLuminance();
	screenquad = new OpenGLScreenQuad();

	// create render targets
	gbuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8B8G8R8_sRGB, GL_NEAREST);	// unused, but needed
	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT1, GLFMT_A16B16G16R16F, GL_NEAREST);	// normals
	gbuffer->Attach(GL_COLOR_ATTACHMENT2, gtaorenderer->GetCurrentDepthBuffer(), 0);
	gbuffer->AttachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GLFMT_D24S8);

	GL_ASSERT(gbuffer->Validate());

#if USE_MSAA
	msaaframebuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	msaaframebuffer->AttachRenderbuffer(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, 4);
	msaaframebuffer->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D32F, 4);

	GL_ASSERT(msaaframebuffer->Validate());
#endif

	framebuffer1 = new OpenGLFramebuffer(screenwidth, screenheight);

	framebuffer1->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);
	framebuffer1->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	GL_ASSERT(framebuffer1->Validate());

	framebuffer2 = new OpenGLFramebuffer(screenwidth, screenheight);

	framebuffer2->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);
	framebuffer2->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	GL_ASSERT(framebuffer2->Validate());

	// setup lights
	lights[LIGHT_ID_SKY] = new PBRLight(PBRLightType::PBRLightTypeEnvProbe);
	lights[LIGHT_ID_LOCAL1] = new PBRLight(PBRLightType::PBRLightTypeEnvProbe);
	lights[LIGHT_ID_SPOT] = new PBRLight(PBRLightType::PBRLightTypeSpot);
	lights[LIGHT_ID_TUBE] = new PBRLight(PBRLightType::PBRLightTypeArea);
	lights[LIGHT_ID_RECT] = new PBRLight(PBRLightType::PBRLightTypeArea);

	Math::Vector3 lightpos = { -1.0f, 3.0f, -2.0f };
	Math::Vector3 lightdir = { 0, -1, 0 };

	// ~40 W
	lights[LIGHT_ID_SPOT]->SetSpotParameters(623, lightpos, lightdir, 9.0f, Math::DegreesToRadians(75), Math::DegreesToRadians(80));
	lights[LIGHT_ID_SPOT]->SetShadowBox(CalculateShadowBox(LIGHT_ID_SPOT));

	// ~10 W
	lightpos = { -2.5f, 3.7f, -2.0f };
	Math::QuaternionRotationAxis(orientation, Math::HALF_PI, 1, 0, 0);

	lights[LIGHT_ID_TUBE]->SetAreaParameters(600, lightpos, orientation, 3, 0, 0.1f);

	// ~2 W
	lightpos = { -2, 2.1f, -4.81f };
	Math::QuaternionRotationAxis(orientation, Math::PI, 1, 0, 0);

	lights[LIGHT_ID_RECT]->SetAreaParameters(120, lightpos, orientation, 4, 0.3f, 0);

	lights[LIGHT_ID_SKY]->SetProbeTextures("../../Media/Textures/uffizi_diff_irrad.dds", "../../Media/Textures/uffizi_spec_irrad.dds");
	lights[LIGHT_ID_LOCAL1]->SetProbeTextures("../../Media/Textures/uffizi_diff_irrad.dds", "../../Media/Textures/local1_spec_irrad.dds");

	// calculate scene bounding box
	for (int i = 0; i < NUM_OBJECTS; ++i) {
		if (objects[i] == nullptr)	//
			continue;	//

		objects[i]->RecalculateBoundingBox();
		const Math::AABox& tmpbox = objects[i]->GetBoundingBox();

		if (objects[i]->GetBody() != nullptr)
			objects[i]->GetBody()->SetUserData(MAKE_DWORD_PTR(i, 0));

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// render shadow map
	lights[LIGHT_ID_SPOT]->RenderShadowMap([](OpenGLEffect* effect) {
		for (int j = 0; j < NUM_OBJECTS; ++j) {
			if (objects[j] != nullptr && IsAffectedByLight(j, LIGHT_ID_SPOT))
				objects[j]->DrawFast(effect);
		}
	});

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// setup camera (don't let it look all the way down cause of VFC limitation)
	camera = new FPSCamera(physicsworld);

	camera->SetFov(Math::DegreesToRadians(50));
	camera->SetAspect((float)screenwidth / (float)screenheight);
	camera->SetPitchLimits(-Math::HALF_PI, Math::HALF_PI - 0.1f);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);
	GLCreateTexture(512, 256, 1, GLFMT_A8B8G8R8, &infotex);

	GLRenderText(
		"Use WASD and mouse to move around\n\n1 - Toggle GTAO\n2 - Toggle gallery lamp\n3 - Toggle debug view\n\nH - Toggle help text",
		helptext, 512, 512);

	debugbox = GLCreateDebugBox();

	ResetCamera();

	return true;
}

void UninitScene()
{
	for (auto& it : materials) {
		delete it.second;
		it.second = nullptr;
	}

	for (int i = 0; i < NUM_OBJECTS; ++i)
		delete objects[i];

	for (int i = 0; i < NUM_LIGHTS; ++i)
		delete lights[i];

	materials.clear();

	delete debugbox;
	delete skymesh;

	delete gbuffer;
	delete msaaframebuffer;
	delete framebuffer1;
	delete framebuffer2;

	delete gtaorenderer;
	delete averageluminance;
	delete screenquad;

	delete simplecolor;
	delete skyeffect;
	delete gbuffereffect;
	delete pbrlightprobe;
	delete pbrspotlight;
	delete pbrarealight;
	delete gtaocombine;
	delete tonemap;

	delete camera;
	delete physicsworld;

	GL_SAFE_DELETE_TEXTURE(environment);
	GL_SAFE_DELETE_TEXTURE(integratedbrdf);
	GL_SAFE_DELETE_TEXTURE(infotex);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyDown(KeyCode key)
{
	camera->OnKeyDown(key);
}

void KeyUp(KeyCode key)
{
	PBRLight* light = nullptr;

	camera->OnKeyUp(key);

	switch (key) {
	case KeyCode1:
		gtaomode = (3 - gtaomode);
		gtaocombine->SetInt("renderMode", gtaomode);
		break;

	case KeyCode2:
		light = lights[LIGHT_ID_RECT];
		light->SetEnabled(!light->IsEnabled());

		// NOTE: if you want to toggle spot light
		//if (light->IsEnabled())
		//	objects[OBJECT_ID_LAMP]->SetMaterial(1, materials["LampInside"]);
		//else
		//	objects[OBJECT_ID_LAMP]->SetMaterial(1, materials["LampInsideOff"]);

		break;

	case KeyCode3:
		debugphysics = !debugphysics;
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	default:
		break;
	}
}

void MouseDown(MouseButton button, int32_t x, int32_t y)
{
	camera->OnMouseDown(button);
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	camera->OnMouseMove(dx, dy);
}

void MouseUp(MouseButton button, int32_t x, int32_t y)
{
	if (button == MouseButtonRight) {
		Math::Matrix viewproj, viewprojinv;
		Math::Vector4 p1, p2;
		Math::Vector3 dir;

		camera->GetViewMatrix(viewproj);
		camera->GetProjectionMatrix(viewprojinv);

		Math::MatrixMultiply(viewproj, viewproj, viewprojinv);
		Math::MatrixInverse(viewprojinv, viewproj);

		// can't project vectors
		p1 = { ((float)x / app->GetClientWidth()) * 2 - 1, 1 - ((float)y / app->GetClientHeight()) * 2, 0, 1 };
		p2 = { p1.x, p1.y, 0.1f, 1 };

		Math::Vec4Transform(p1, p1, viewprojinv);
		Math::Vec4Transform(p2, p2, viewprojinv);

		p1 /= p1.w;
		p2 /= p2.w;

		Math::Vec3Subtract(dir, (const Math::Vector3&)p2, (const Math::Vector3&)p1);
		Math::Vec3Normalize(dir, dir);

		RigidBody* body = physicsworld->RayIntersect((const Math::Vector3&)p1, dir);

		if (body != nullptr) {
			if (body != selectedbody) {
				// select
				selectedbody = body;

				uint64_t data = reinterpret_cast<uint64_t>(selectedbody->GetUserData());
				SceneObject* selectedobject = objects[(data >> 16) & 0x0000ffffULL];

				if (selectedobject != nullptr)
					UpdateMaterialInfo(*selectedobject->GetMaterial(data & 0x0000ffffULL));
			} else {
				// deselect
				selectedbody = nullptr;
			}
		} else {
			// deselect
			selectedbody = nullptr;
		}
	}

	camera->OnMouseUp(button);
}

void ResetCamera()
{
	// NOTE: test position for article
	//camera->SetEyePosition(-2.43f, 1.8f, 0.681f);
	//camera->SetOrientation(6.42f, 0.16f, 0);

	// NOTE: test position for physics
	//camera->SetEyePosition(-4.7f, 4.0f, 0.1f);
	//camera->SetOrientation(6.42f, 0.16f, 0);

	camera->SetEyePosition(1.5f, 1.8f, -0.225f);
	camera->SetOrientation(5.15f, 0.12f, 0);
}

void Update(float delta)
{
	Math::Vector3 campos, center;

	camera->Update(delta);

	camera->GetEyePosition(campos);
	scenebox.GetCenter(center);

	if (Math::Vec3Distance(campos, center) > scenebox.Radius() * 1.5f)
		ResetCamera();
}

void CullScene(const Math::Matrix& viewproj)
{
	Math::Vector4 planes[6] = {
		{ 1, 0, 0, 1 },
		{ -1, 0, 0, 1 },
		{ 0, 1, 0, 1 },
		{ 0, -1, 0, 1 },
		{ 0, 0, 1, 0 },
		{ 0, 0, -1, 1 },
	};

	for (int i = 0; i < 6; ++i) {
		Math::PlaneTransformTranspose(planes[i], viewproj, planes[i]);
		Math::PlaneNormalize(planes[i], planes[i]);
	}

	Math::Vector3	center;
	Math::Vector3	halfsize;
	float			dist, maxdist;
	int				numculled = 0;
	bool			visible;

	// NOTE: this is fast, but might give false negative for concave objects
	for (int i = 0; i < NUM_OBJECTS; ++i) {
		if (objects[i] == nullptr)
			continue;

		const Math::AABox& box = objects[i]->GetBoundingBox();

		box.GetCenter(center);
		box.GetHalfSize(halfsize);

		visible = true;

		for (int j = 0; j < 6; ++j) {
			const Math::Vector4& plane = planes[j];

			dist = Math::PlaneDotCoord(plane, center);
			maxdist = (fabs(plane.x * halfsize.x) + fabs(plane.y * halfsize.y) + fabs(plane.z * halfsize.z));

			if (dist < -maxdist) {
				// outside
				visible = false;
				++numculled;

				break;
			}
		}

		objects[i]->SetVisible(visible);
	}
}

void RenderScene(const Math::Matrix& viewproj, const Math::Vector3& eye, bool transparent, LightMode mode)
{
	Math::Matrix lightviewproj;
	OpenGLEffect* effect = nullptr;
	int start = 0;
	int end = NUM_LIGHTS;

	if (mode == LightModeSkyOnly)
		end = 1;
	else if (mode == LightModeAllExceptSky)
		start = 1;

	glEnable(GL_BLEND);

	if (transparent)
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	else
		glBlendFunc(GL_ONE, GL_ONE);

	for (int i = start; i < end; ++i) {
		const PBRLight& light = *lights[i];

		if (!light.IsEnabled())
			continue;

		switch (light.GetType()) {
		case PBRLightTypeSpot:
			effect = pbrspotlight;

			light.GetViewProj(lightviewproj);

			effect->SetMatrix("lightViewProj", lightviewproj);
			effect->SetVector("lightPos", light.GetPosition());
			effect->SetVector("lightDir", light.GetDirection());
			effect->SetVector("clipPlanes", light.GetClipPlanes());

			effect->SetFloat("lumIntensity", light.GetLuminuousIntensity());
			effect->SetFloat("angleScale", light.GetSpotAngleScale());
			effect->SetFloat("angleOffset", light.GetSpotAngleOffset());
			effect->SetFloat("invRadius", light.GetInvRadius());

			GLSetTexture(GL_TEXTURE2, GL_TEXTURE_2D, light.GetShadowMap()->GetColorAttachment(0));
			break;

		case PBRLightTypeEnvProbe:
			effect = pbrlightprobe;

			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_CUBE_MAP, light.GetDiffuseIlluminance());
			GLSetTexture(GL_TEXTURE2, GL_TEXTURE_CUBE_MAP, light.GetSpecularIlluminance());
			GLSetTexture(GL_TEXTURE3, GL_TEXTURE_2D, integratedbrdf);
			
			break;

		case PBRLightTypeArea:
			effect = pbrarealight;

			effect->SetVector("lightPos", light.GetPosition());
			effect->SetVector("lightRight", light.GetExtentRight());
			effect->SetVector("lightUp", light.GetExtentUp());

			effect->SetFloat("luminance", light.GetLuminance());
			effect->SetFloat("radius", light.GetRadius());

			break;

		default:
			continue;
		}

		// render objects that the current light affects
		if (effect != nullptr) {
			effect->SetVector("eyePos", eye);
			effect->SetMatrix("matViewProj", viewproj);

			effect->Begin();
			{
				for (int j = 0; j < NUM_OBJECTS; ++j) {
					if (objects[j] != nullptr && IsAffectedByLight(j, i))
						objects[j]->Draw(effect, transparent);
				}
			}
			effect->End();
		}

		if (i == LIGHT_ID_SKY) {
			// NOTE: incorrect; assumes that all transparents are affected by first light
			glBlendFunc(GL_ONE, GL_ONE);
		}
	}

	glDisable(GL_BLEND);
}

void RenderSky(const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector3& eye)
{
	Math::Matrix world, tmp(proj);

	// project to depth ~ 1.0
	tmp._33 = -1.0f + 1e-4f;
	tmp._43 = 0;

	Math::MatrixTranslation(world, eye.x, eye.y, eye.z);
	Math::MatrixMultiply(tmp, view, tmp);

	skyeffect->SetVector("eyePos", eye);
	skyeffect->SetMatrix("matWorld", world);
	skyeffect->SetMatrix("matViewProj", tmp);

	skyeffect->Begin();
	{
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, environment);
		skymesh->DrawSubset(0);
	}
	skyeffect->End();
}

void RenderLights()
{
	Math::Matrix world;
	Math::Vector4 color;

	simplecolor->Begin();
	{
		for (int i = 0; i < NUM_LIGHTS; ++i) {
			const PBRLight& light = *lights[i];

			switch (light.GetType()) {
			case PBRLightTypeSpot:
			case PBRLightTypeArea:
				Math::MatrixRotationQuaternion(world, light.GetOrientation());

				world._41 = light.GetPosition().x;
				world._42 = light.GetPosition().y;
				world._43 = light.GetPosition().z;

				if (light.IsEnabled())
					color = { light.GetLuminance(), light.GetLuminance(), light.GetLuminance(), 1 };
				else
					color = { 0.3f, 0.3f, 0.3f, 1 };

				simplecolor->SetMatrix("matWorld", world);
				simplecolor->SetVector("matColor", color);
				break;

			default:
				continue;
			}

			simplecolor->CommitChanges();
			light.GetGeometry()->DrawSubset(0);
		}
	}
	simplecolor->End();
}

void Render(float alpha, float elapsedtime)
{
#if USE_MSAA
	OpenGLFramebuffer* rendertarget = msaaframebuffer;
#else
	OpenGLFramebuffer* rendertarget = framebuffer1;
#endif

	Math::Matrix world, view, proj;
	Math::Matrix viewinv, viewproj;
	Math::Vector4 clipinfo;	// TODO: prevclipinfo
	Math::Vector3 eye;

	camera->Animate(alpha);
	camera->FitToBox(scenebox);

	camera->GetViewMatrix(view);
	camera->GetProjectionMatrix(proj);
	camera->GetEyePosition(eye);

	Math::MatrixInverse(viewinv, view);
	Math::MatrixMultiply(viewproj, view, proj);

	clipinfo.x = camera->GetNearPlane();
	clipinfo.y = camera->GetFarPlane();
	clipinfo.z = 0.5f * (app->GetClientHeight() / (2.0f * tanf(camera->GetFov() * 0.5f)));

	// view frustum culling
	CullScene(viewproj);

	Math::Color clearcolor	= { 0, 0.0103f, 0.0707f, 1 };
	Math::Color black		= { 0, 0, 0, 1 };
	Math::Color white		= { 1, 1, 1, 1 };

	// STEP 1: render g-buffer for GTAO
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	gbuffereffect->SetMatrix("matView", view);
	gbuffereffect->SetMatrix("matViewInv", viewinv);
	gbuffereffect->SetMatrix("matProj", proj);
	gbuffereffect->SetVector("clipPlanes", clipinfo);

	gbuffer->Detach(GL_COLOR_ATTACHMENT2);
	gbuffer->Attach(GL_COLOR_ATTACHMENT2, gtaorenderer->GetCurrentDepthBuffer(), 0);

	gbuffer->Set();
	{
		glClearBufferfv(GL_COLOR, 0, clearcolor);
		glClearBufferfv(GL_COLOR, 1, black);
		glClearBufferfv(GL_COLOR, 2, white);
		glClearBufferfv(GL_DEPTH, 0, white);

		gbuffereffect->Begin();
		{
			for (int i = 0; i < NUM_OBJECTS; ++i) {
				if (objects[i] != nullptr)
					objects[i]->Draw(gbuffereffect, false);
			}
		}
		gbuffereffect->End();
	}
	gbuffer->Unset();

	// STEP 2: render GTAO
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	gtaorenderer->Render(gbuffer->GetColorAttachment(1), view, proj, eye, clipinfo);

	// STEP 3: z-only pass
	rendertarget->Set();
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		simplecolor->SetMatrix("matViewProj", viewproj);

		simplecolor->Begin();
		{
			for (int i = 0; i < NUM_OBJECTS; ++i) {
				if (objects[i] != nullptr)
					objects[i]->DrawFast(simplecolor);
			}
		}
		simplecolor->End();

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
	}

	// STEP 4: render opaque objects with sky
	{
		RenderScene(viewproj, eye, false, LightModeSkyOnly);
		RenderSky(view, proj, eye);
	}
	rendertarget->Unset();

#if USE_MSAA
	msaaframebuffer->Resolve(framebuffer1, GL_COLOR_BUFFER_BIT);
	framebuffer1->Unset();

	rendertarget = msaaframebuffer;
#else
	framebuffer1->CopyDepthStencil(framebuffer2, GL_DEPTH_BUFFER_BIT);
	rendertarget = framebuffer2;
#endif

	// STEP 5: apply GTAO
	rendertarget->Set();
	{
		glDisable(GL_DEPTH_TEST);

		gtaocombine->Begin();
		{
			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, framebuffer1->GetColorAttachment(0));
			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, gtaorenderer->GetGTAO());

			screenquad->Draw(false);
		}
		gtaocombine->End();

		glEnable(GL_DEPTH_TEST);
	}

	// STEP 6: render opaque objects with other lights
	{
		RenderScene(viewproj, eye, false, LightModeAllExceptSky);
		RenderLights();
	}

	// STEP 7: render transparent objects
	{
		RenderScene(viewproj, eye, true, LightModeAll);
	}
	rendertarget->Unset();

#if USE_MSAA
	msaaframebuffer->Resolve(framebuffer2, GL_COLOR_BUFFER_BIT);
	framebuffer2->Unset();
#endif

	// STEP 8: measure & adapt luminance
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	averageluminance->Measure(framebuffer2->GetColorAttachment(0), Math::Vector2(1.0f / screenwidth, 1.0f / screenheight), elapsedtime);
	glViewport(0, 0, screenwidth, screenheight);

	// STEP 9: tone map
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	tonemap->Begin();
	{
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, framebuffer2->GetColorAttachment(0));
		GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, averageluminance->GetAverageLuminance());

		screenquad->Draw(false);
	}
	tonemap->End();

	// debug render
	if (debugphysics) {
		glDisable(GL_DEPTH_TEST);

		simplecolor->SetMatrix("matViewProj", viewproj);
		simplecolor->Begin();
		{
			physicsworld->DEBUG_Visualize(&DEBUG_DrawBody);
		}
		simplecolor->End();
	}

	// material info
	Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
	Math::MatrixReflect(world, xzplane);

	screenquad->SetTextureMatrix(world);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FRAMEBUFFER_SRGB);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (selectedbody != nullptr) {
		Math::AABox box;
		Math::Vector4 center;

		selectedbody->GetTransformWithSize(world);

		center = { world._41, world._42, world._43, 1.0f };
		Math::Vec4Transform(center, center, viewproj);

		if (center.w > camera->GetNearPlane()) {
			center.x = (center.x / center.w * 0.5f + 0.5f) * app->GetClientWidth();
			center.y = (center.y / center.w * 0.5f + 0.5f) * app->GetClientHeight();

			glViewport((GLsizei)center.x, (GLsizei)center.y, 512, 256);
			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, infotex);

			screenquad->Draw();
		}
	}

	if (drawtext) {
		// render text
		glViewport(10, screenheight - 522, 512, 512);
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, helptext);
	
		screenquad->Draw();
	}

	// reset states
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glViewport(0, 0, screenwidth, screenheight);

	// check errors
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		std::cout << "Error\n";

	app->Present();
}

void DEBUG_DrawBody(RigidBodyType type, const Math::Matrix& xform)
{
	simplecolor->SetMatrix("matWorld", xform);
	simplecolor->CommitChanges();

	debugbox->DrawSubset(0);
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGL)) {
		delete app;
		return 1;
	}

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyDownCallback = KeyDown;
	app->KeyUpCallback = KeyUp;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;
	app->MouseMoveCallback = MouseMove;

	app->Run();
	delete app;

	return 0;
}
