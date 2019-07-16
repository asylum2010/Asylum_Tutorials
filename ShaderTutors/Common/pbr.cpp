
#include "pbr.h"
#include "gl4ext.h"
#include "geometryutils.h"

static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not 32 bit");

PBRLight::PBRLight(PBRLightType type)
{
	this->type			= type;
	lumintensity		= 1;
	luminance			= 1;
	invradius			= 1;

	geometry			= nullptr;
	shadowmap			= nullptr;
	blurredshadowmap	= nullptr;
	blockereffect		= nullptr;
	screenquad			= nullptr;

	diffillum			= 0;
	specillum			= 0;
	shadowmapsize		= 1024;
	enabled				= true;

	Math::QuaternionIdentity(orientation);

	right = Math::Vector3(1, 0, 0);
	up = Math::Vector3(0, 1, 0);
	forward = Math::Vector3(0, 1, 0);
	
	spotangles[0] = cosf(Math::DegreesToRadians(30));
	spotangles[1] = cosf(Math::DegreesToRadians(45));
}

PBRLight::~PBRLight()
{
	delete geometry;
	delete shadowmap;
	delete blurredshadowmap;
	delete blockereffect;
	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(diffillum);
	GL_SAFE_DELETE_TEXTURE(specillum);
}

void PBRLight::RenderShadowMap(std::function<void (OpenGLEffect*)> callback)
{
	if (shadowmap == nullptr || blockereffect == nullptr)
		return;

	Math::Matrix viewproj;

	GetViewProj(viewproj);

	blockereffect->SetMatrix("matViewProj", viewproj);
	blockereffect->SetVector("clipPlanes", spotclip);
	blockereffect->SetInt("isPerspective", 1);

	shadowmap->Set();
	{
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		blockereffect->Begin();
		{
			callback(blockereffect);
		}
		blockereffect->End();
	}
	shadowmap->Unset();

	// blur shadow map
	glDisable(GL_DEPTH_TEST);

	blurredshadowmap->Set();
	{
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));
		screenquad->Draw();
	}
	blurredshadowmap->Unset();
}

void PBRLight::SetSpotParameters(float lumflux, const Math::Vector3& pos, const Math::Vector3& dir, float radius, float inner, float outer)
{
	Math::Matrix identity(1, 1, 1, 1);
	Math::Vector4 texelsize(1.0f / shadowmapsize, 1.0f / shadowmapsize, 0, 0);

	position = pos;
	forward = dir;

	spotangles[0] = cosf(inner);
	spotangles[1] = cosf(outer);

	// NOTE: assume variance shadow
	GLCreateEffectFromFile("../../Media/ShadersGL/shadowmap_variance.vert", 0, 0, 0, "../../Media/ShadersGL/shadowmap_variance.frag", &blockereffect);
	
	screenquad = new OpenGLScreenQuad("../../Media/ShadersGL/boxblur3x3.frag");
	
	screenquad->SetTextureMatrix(identity);
	screenquad->GetEffect()->SetVector("texelSize", texelsize);

	shadowmap = new OpenGLFramebuffer(shadowmapsize, shadowmapsize);

	shadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);
	shadowmap->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);
	shadowmap->Validate();

	blurredshadowmap = new OpenGLFramebuffer(shadowmapsize, shadowmapsize);

	blurredshadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);
	blurredshadowmap->Validate();

	lumintensity = lumflux / (2 * Math::PI * (1 - cosf(outer)));
	luminance = lumflux / (2 * Math::PI * Math::PI * (1 - cosf(outer)) * 1e-4f); // assume 1 cm radius
	invradius = ((radius == 0) ? FLT_MAX : 1.0f / radius);

	OpenGLVertexElement elem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
		{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	uint32_t numvertices = 0;
	uint32_t numindices = 0;

	GeometryUtils::NumVerticesIndicesSphere(numvertices, numindices, 16, 16);
	GLCreateMesh(numvertices, numindices, GLMESH_32BIT, elem, &geometry);

	GeometryUtils::CommonVertex* vdata = nullptr;
	uint32_t* idata = nullptr;

	geometry->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	geometry->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		GeometryUtils::CreateSphere(vdata, idata, 0.02f, 16, 16);
	}
	geometry->UnlockIndexBuffer();
	geometry->UnlockVertexBuffer();
}

void PBRLight::SetProbeTextures(const char* diffuse, const char* specular)
{
	GLCreateCubeTextureFromDDS(diffuse, false, &diffillum);
	GLCreateCubeTextureFromDDS(specular, false, &specillum);

	lumintensity = 1;
}

void PBRLight::SetAreaParameters(float lumflux, const Math::Vector3& pos, const Math::Quaternion& orient, float width, float height, float radius)
{
	position = pos;
	orientation = orient;

	invradius = ((radius == 0) ? FLT_MAX : 1.0f / radius);

	OpenGLVertexElement elem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
		{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	uint32_t numvertices = 0;
	uint32_t numindices = 0;

	if (width == 0 && height == 0) {
		// TODO: sphere
	} else if (width != 0 && height != 0) {
		// rectangle

		// NOTE: these depend on how you generate the geometry
		right = { 1, 0, 0 };
		up = { 0, 0, -1 };

		Math::Vec3Rotate(right, right, orient);
		Math::Vec3Rotate(up, up, orient);

		Math::Vec3Scale(right, right, width * 0.5f);
		Math::Vec3Scale(up, up, height * 0.5f);

		GLCreateMesh(4, 6, GLMESH_32BIT, elem, &geometry);

		GeometryUtils::CommonVertex* vdata = nullptr;
		uint32_t* idata = nullptr;

		geometry->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
		geometry->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
		{
			GeometryUtils::CreatePlane(vdata, idata, width, height);
		}
		geometry->UnlockIndexBuffer();
		geometry->UnlockVertexBuffer();

		lumintensity = 0;
		luminance = lumflux / (Math::PI * width * height);
	} else {
		// capsule
		float length = Math::Max(width, height);

		// NOTE: these depend on how you generate the geometry
		right = { 0, 1, 0 };
		up = { 1, 0, 0 };

		Math::Vec3Rotate(right, right, orient);
		Math::Vec3Rotate(up, up, orient);

		Math::Vec3Scale(right, right, length * 0.5f);
		Math::Vec3Scale(up, up, radius);

		GeometryUtils::NumVerticesIndicesCapsule(numvertices, numindices, 16, 16);
		GLCreateMesh(numvertices, numindices, GLMESH_32BIT, elem, &geometry);

		GeometryUtils::CommonVertex* vdata = nullptr;
		uint32_t* idata = nullptr;

		geometry->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
		geometry->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
		{
			GeometryUtils::CreateCapsule(vdata, idata, length, radius, 16, 16);
		}
		geometry->UnlockIndexBuffer();
		geometry->UnlockVertexBuffer();

		lumintensity = 0;
		luminance = lumflux / (2 * radius * Math::PI * Math::PI * (length + 2 * radius));
	}
}

void PBRLight::SetShadowBox(const Math::AABox& box)
{
	shadowbox = box;
	Math::FitToBoxPerspective(spotclip, position, forward, box);
}

void PBRLight::GetViewProj(Math::Matrix& out) const
{
	if (type == PBRLightTypeSpot) {
		Math::Matrix view, proj;
		Math::Vector3 look;
		Math::Vector3 up = { 0, 1, 0 };

		if (fabs(forward.y) > 0.99f)
			up = { 0, 0, 1 };

		Math::Vec3Add(look, position, forward);

		Math::MatrixLookAtRH(view, position, look, up);
		Math::MatrixPerspectiveFovRH(proj, acosf(spotangles.y) * 2.0f, 1, spotclip.x, spotclip.y);
		Math::MatrixMultiply(out, view, proj);
	}
}

float PBRLight::GetSpotAngleScale() const
{
	return 1.0f / Math::Max<float>(0.001f, spotangles[0] - spotangles[1]);
}

float PBRLight::GetSpotAngleOffset() const
{
	return -spotangles[1] * GetSpotAngleScale();
}

PBRMaterial::PBRMaterial()
{
	BaseColor	= 0xffffffff;
	Roughness	= 1.0f;
	Metalness	= 0.0f;
	Texture		= 0;
	Transparent	= false;

	texturename	= L"<texture>";
}

PBRMaterial::PBRMaterial(const std::wstring& name, Math::Color color, float roughness, float metalness, const char* texture, bool transparent)
{
	Name		= name;
	BaseColor	= color;
	Roughness	= roughness;
	Metalness	= metalness;
	Texture		= 0;
	Transparent	= transparent;

	texturename	= L"<texture>";

	if (texture != nullptr) {
#ifdef _WIN32
		std::string file;
		Math::GetFile(file, texture);

		int size = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), 0, 0);

		texturename.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), &texturename[0], size);
#else
		// TODO:
#endif

		GLCreateTextureFromFile(texture, true, &Texture);
	}
}

PBRMaterial::~PBRMaterial()
{
	GL_SAFE_DELETE_TEXTURE(Texture);
}
