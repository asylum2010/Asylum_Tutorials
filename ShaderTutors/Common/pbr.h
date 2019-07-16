
#ifndef _PBR_H_
#define _PBR_H_

#include <unordered_map>
#include <functional>

#include "3Dmath.h"

class OpenGLMesh;
class OpenGLFramebuffer;
class OpenGLEffect;
class OpenGLScreenQuad;

enum PBRLightType
{
	PBRLightTypePoint = 0,
	PBRLightTypeSpot,
	PBRLightTypeEnvProbe,
	PBRLightTypeArea
};

class PBRLight
{
private:
	Math::AABox			shadowbox;
	Math::Quaternion	orientation;	// when area light
	Math::Vector3		position;
	Math::Vector3		right;			// area light right extent
	Math::Vector3		up;				// area light up extent
	Math::Vector3		forward;		// when spot light
	Math::Vector2		spotangles;		// cos(inner), cos(outer)
	Math::Vector2		spotclip;		// near, far

	OpenGLMesh*			geometry;
	OpenGLFramebuffer*	shadowmap;
	OpenGLFramebuffer*	blurredshadowmap;
	OpenGLScreenQuad*	screenquad;		// to blur shadow map
	OpenGLEffect*		blockereffect;	// to render shadow map
	uint32_t			diffillum;		// when light probe
	uint32_t			specillum;		// when light probe
	uint32_t			shadowmapsize;

	float				lumintensity;
	float				luminance;
	float				invradius;
	PBRLightType		type;
	bool				enabled;

public:
	PBRLight(PBRLightType type);
	~PBRLight();

	void RenderShadowMap(std::function<void (OpenGLEffect*)> callback);

	void SetSpotParameters(float lumflux, const Math::Vector3& pos, const Math::Vector3& dir, float radius, float inner, float outer);
	void SetProbeTextures(const char* diffuse, const char* specular);
	void SetAreaParameters(float lumflux, const Math::Vector3& pos, const Math::Quaternion& orient, float width, float height, float radius);
	void SetShadowBox(const Math::AABox& box);

	void GetViewProj(Math::Matrix& out) const;

	float GetSpotAngleScale() const;
	float GetSpotAngleOffset() const;

	inline void SetEnabled(bool enable)						{ enabled = enable; }
	inline bool IsEnabled() const							{ return enabled; }

	inline PBRLightType GetType() const						{ return type; }
	inline uint32_t GetDiffuseIlluminance() const			{ return diffillum; }
	inline uint32_t GetSpecularIlluminance() const			{ return specillum; }

	inline OpenGLMesh* GetGeometry() const					{ return geometry; }
	inline OpenGLFramebuffer* GetShadowMap() const			{ return blurredshadowmap; }

	inline const Math::Vector3& GetPosition() const			{ return position; }
	inline const Math::Vector3& GetDirection() const		{ return forward; }
	inline const Math::Vector3& GetExtentRight() const		{ return right; }
	inline const Math::Vector3& GetExtentUp() const			{ return up; }
	inline const Math::Quaternion& GetOrientation() const	{ return orientation; }
	inline const Math::Vector2& GetClipPlanes() const		{ return spotclip; }

	inline float GetLuminuousIntensity() const				{ return lumintensity; }
	inline float GetLuminance() const						{ return luminance; }
	inline float GetInvRadius() const						{ return invradius; }
	inline float GetRadius() const							{ return ((invradius == FLT_MAX) ? 0 : 1.0f / invradius); }
};

class PBRMaterial
{
private:
	std::wstring texturename;

public:
	std::wstring	Name;
	Math::Color		BaseColor;
	float			Roughness;
	float			Metalness;
	uint32_t		Texture;
	bool			Transparent;

	PBRMaterial();
	PBRMaterial(const std::wstring& name, Math::Color color, float roughness, float metalness, const char* texture, bool transparent = false);
	~PBRMaterial();

	inline const std::wstring& GetTextureName() const	{ return texturename; }
};

typedef std::unordered_map<std::string, PBRMaterial*> PBRMaterialTable;

#endif
