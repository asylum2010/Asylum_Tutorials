
#ifndef _GL4EXT_H_
#define _GL4EXT_H_

#include <map>
#include <vector>
#include <cassert>

#include "glextensions.h"
#include "3Dmath.h"
#include "orderedarray.hpp"

#ifndef WCHAR
#	define WCHAR wchar_t
#endif

#ifdef _DEBUG
#	define GL_ASSERT(x)	assert(x)
#else
#	define GL_ASSERT(x)	{ if (!(x)) { throw 1; } }
#endif

#define GL_SAFE_DELETE_BUFFER(x) \
	if (x != 0) { \
		glDeleteBuffers(1, &x); \
		x = 0; }
// END

#define GL_SAFE_DELETE_TEXTURE(x) \
	if (x != 0) { \
		glDeleteTextures(1, &x); \
		x = 0; }
// END

#if defined(__APPLE__)
// NOTE: to get files from bundle
std::string GetResource(const std::string& file);
#endif

// --- Enums ------------------------------------------------------------------

enum OpenGLDeclType
{
	GLDECLTYPE_FLOAT1 =  0,
	GLDECLTYPE_FLOAT2 =  1,
	GLDECLTYPE_FLOAT3 =  2,
	GLDECLTYPE_FLOAT4 =  3,
	GLDECLTYPE_GLCOLOR =  4
};

enum OpenGLDeclUsage
{
	GLDECLUSAGE_POSITION = 0,
	GLDECLUSAGE_BLENDWEIGHT,
	GLDECLUSAGE_BLENDINDICES,
	GLDECLUSAGE_NORMAL,
	GLDECLUSAGE_PSIZE,
	GLDECLUSAGE_TEXCOORD,
	GLDECLUSAGE_TANGENT = GLDECLUSAGE_TEXCOORD + 8,
	GLDECLUSAGE_BINORMAL,
	GLDECLUSAGE_TESSFACTOR,
	GLDECLUSAGE_POSITIONT,
	GLDECLUSAGE_COLOR,
	GLDECLUSAGE_FOG,
	GLDECLUSAGE_DEPTH,
	GLDECLUSAGE_SAMPLE
};

enum OpenGLFormat
{
	GLFMT_UNKNOWN = 0,
	GLFMT_R8,
	GLFMT_R8G8,
	GLFMT_R8G8B8,
	GLFMT_R8G8B8_sRGB,
	GLFMT_B8G8R8,
	GLFMT_B8G8R8_sRGB,
	GLFMT_A8R8G8B8,
	GLFMT_A8R8G8B8_sRGB,
	GLFMT_A8B8G8R8,
	GLFMT_A8B8G8R8_sRGB,

	GLFMT_D24S8,
	GLFMT_D32F,

	GLFMT_DXT1,
	GLFMT_DXT1_sRGB,
	GLFMT_DXT5,
	GLFMT_DXT5_sRGB,

	GLFMT_R16F,
	GLFMT_G16R16F,
	GLFMT_A16B16G16R16F,

	GLFMT_R32F,
	GLFMT_G32R32F,
	GLFMT_A32B32G32R32F
};

enum OpenGLPrimitiveType
{
	GLPT_POINTLIST = GL_POINTS,
	GLPT_LINELIST = GL_LINES,
	GLPT_TRIANGLELIST = GL_TRIANGLES
};

enum OpenGLLockFlags
{
	GLLOCK_READONLY = GL_MAP_READ_BIT,
	GLLOCK_DISCARD = GL_MAP_INVALIDATE_RANGE_BIT|GL_MAP_WRITE_BIT
};

enum OpenGLMeshFlags
{
	GLMESH_DYNAMIC = 1,
	GLMESH_32BIT = 2
};

enum OpenGLTextureFlag
{
	GLTEX_FLIPX = 1,
	GLTEX_FLIPY = 2
};

enum OpenGLLoadAction
{
	GLLOADACTION_DONTCARE = 0,
	GLLOADACTION_LOAD,
	GLLOADACTION_CLEAR
};

enum OpenGLStoreAction
{
	GLSTOREACTION_DONTCARE = 0,
	GLSTOREACTION_STORE,
	GLSTOREACTION_RESOLVE,
	GLSTOREACTION_STORE_AND_RESOLVE
};

// --- Structures -------------------------------------------------------------

struct OpenGLVertexElement
{
	GLushort	Stream;
	GLushort	Offset;
	GLubyte		Type;			// OpenGLDeclType
	GLubyte		Usage;			// OpenGLDeclUsage
	GLubyte		UsageIndex;
};

struct OpenGLVertexDeclaration
{
	OpenGLVertexElement*	Elements;
	GLuint					Stride;
};

struct OpenGLAttributeRange
{
	GLenum		PrimitiveType;	// OpenGLPrimitiveType
	GLuint		AttribId;
	GLuint		IndexStart;
	GLuint		IndexCount;
	GLuint		VertexStart;
	GLuint		VertexCount;
	GLboolean	Enabled;
};

struct OpenGLMaterial
{
	Math::Color	Diffuse;
	Math::Color	Ambient;
	Math::Color	Specular;
	Math::Color	Emissive;
	float		Power;
	GLuint		Texture;
	GLuint		NormalMap;

	OpenGLMaterial();
	~OpenGLMaterial();
};

// --- Classes ----------------------------------------------------------------

class OpenGLContentRegistry
{
	typedef std::map<std::string, GLuint> TextureMap;

private:
	static OpenGLContentRegistry* _inst;

	TextureMap textures;

	OpenGLContentRegistry();
	~OpenGLContentRegistry();

public:
	static OpenGLContentRegistry& Instance();
	static void Release();

	void RegisterTexture(const std::string& file, GLuint tex);
	void UnregisterTexture(GLuint tex);

	GLuint IDTexture(const std::string& file);
};

inline OpenGLContentRegistry& OpenGLContentManager() {
	return OpenGLContentRegistry::Instance();
}

/**
 * \brief Similar to ID3DXMesh. One stream, core profile only.
 */
class OpenGLMesh
{
	friend bool GLCreateMesh(GLuint, GLuint, GLuint, OpenGLVertexElement*, OpenGLMesh**);
	friend bool GLCreateMeshFromQM(const char*, OpenGLMesh**, uint32_t);

	struct LockedData
	{
		void* ptr;
		GLuint flags;
	};

private:
	Math::AABox					boundingbox;
	OpenGLAttributeRange*		subsettable;
	OpenGLMaterial*				materials;
	OpenGLVertexDeclaration		vertexdecl;
	
	GLuint						meshoptions;
	GLuint						numsubsets;
	GLuint						numvertices;
	GLuint						numindices;
	GLuint						vertexbuffer;
	GLuint						indexbuffer;
	GLuint						vertexlayout;

	LockedData					vertexdata_locked;
	LockedData					indexdata_locked;

	OpenGLMesh();

	void Destroy();
	void RecreateVertexLayout();

public:
	~OpenGLMesh();

	bool LockVertexBuffer(GLuint offset, GLuint size, GLuint flags, void** data);
	bool LockIndexBuffer(GLuint offset, GLuint size, GLuint flags, void** data);

	void Draw();
	void DrawInstanced(GLuint numinstances);
	void DrawSubset(GLuint subset, bool bindtextures = false);
	void DrawSubsetInstanced(GLuint subset, GLuint numinstances, bool bindtextures = false);
	void EnableSubset(GLuint subset, bool enable);
	void GenerateTangentFrame();
	void ReorderSubsets(GLuint newindices[]);
	void UnlockVertexBuffer();
	void UnlockIndexBuffer();
	void SetAttributeTable(const OpenGLAttributeRange* table, GLuint size);

	inline void SetBoundingBox(const Math::AABox& box)	{ boundingbox = box; }
	inline OpenGLAttributeRange* GetAttributeTable()	{ return subsettable; }
	inline OpenGLMaterial* GetMaterialTable()			{ return materials; }
	inline const Math::AABox& GetBoundingBox() const	{ return boundingbox; }
	inline size_t GetNumBytesPerVertex() const			{ return vertexdecl.Stride; }
	inline GLuint GetNumSubsets() const					{ return numsubsets; }
	inline GLuint GetNumVertices() const				{ return numvertices; }
	inline GLuint GetNumIndices() const					{ return numindices; }
	inline GLuint GetVertexLayout() const				{ return vertexlayout; }
	inline GLuint GetVertexBuffer() const				{ return vertexbuffer; }
	inline GLuint GetIndexBuffer() const				{ return indexbuffer; }
	inline GLenum GetIndexType() const					{ return ((meshoptions & GLMESH_32BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT); }
};

/**
 * \brief Similar to ID3DXEffect. One technique, core profile only.
 */
class OpenGLEffect
{
	friend bool GLCreateEffectFromFile(const char*, const char*, const char*, const char*, const char*, OpenGLEffect**, const char*);
	//friend bool GLCreateEffectFromMemory(const char*, const char*, const char*, const char*, const char*, OpenGLEffect**, const char*);
	friend bool GLCreateComputeProgramFromFile(const char*, OpenGLEffect**, const char*);

	struct Uniform
	{
		char	Name[32];
		GLint	StartRegister;
		GLint	RegisterCount;
		GLint	Location;
		GLenum	Type;

		mutable bool Changed;

		inline bool operator <(const Uniform& other) const {
			return (0 > strcmp(Name, other.Name));
		}
	};

	struct UniformBlock
	{
		char			Name[32];
		GLint			Index;
		mutable GLint	Binding;
		GLint			BlockSize;

		inline bool operator <(const UniformBlock& other) const {
			return (0 > strcmp(Name, other.Name));
		}
	};

	typedef OrderedArray<Uniform> UniformTable;
	typedef OrderedArray<UniformBlock> UniformBlockTable;

private:
	UniformTable		uniforms;
	UniformBlockTable	uniformblocks;
	GLuint				program;

	float*				floatvalues;
	int*				intvalues;
	GLuint*				uintvalues;

	uint32_t			floatcap;
	uint32_t			floatsize;
	uint32_t			intcap;
	uint32_t			intsize;
	uint32_t			uintcap;
	uint32_t			uintsize;

	OpenGLEffect();

	void AddUniform(const char* name, GLuint location, GLuint count, GLenum type);
	void AddUniformBlock(const char* name, GLint index, GLint binding, GLint blocksize);
	void BindAttributes();
	void Destroy();
	void QueryUniforms();

public:
	~OpenGLEffect();

	void Begin();
	void CommitChanges();
	void End();
	void Introspect();

	void SetMatrix(const char* name, const float* value);
	void SetVector(const char* name, const float* value);
	void SetVectorArray(const char* name, const float* values, GLsizei count);
	void SetFloat(const char* name, float value);
	void SetFloatArray(const char* name, const float* values, GLsizei count);
	void SetInt(const char* name, int value);
	void SetUIntVector(const char* name, const GLuint* value);
	void SetUniformBlockBinding(const char* name, GLint binding);
};

/**
 * \brief Uses the new program pipeline API
 */
class OpenGLProgramPipeline
{
private:
	GLuint	shaders[5];
	GLuint	pipeline;
	GLuint	program;

public:
	OpenGLProgramPipeline();
	~OpenGLProgramPipeline();

	bool AddShader(GLenum type, const char* file, const char* defines = nullptr);
	bool Assemble();
	
	void Bind();
	void UseProgramStages(OpenGLProgramPipeline* other, GLbitfield stages);

	inline GLuint GetProgram() const	{ return program; }
};

/**
 * \brief Wrapper for framebuffer object
 */
class OpenGLFramebuffer
{
	enum AttachmentType
	{
		GL_ATTACH_TYPE_RENDERBUFFER = 0,
		GL_ATTACH_TYPE_TEXTURE,
		GL_ATTACH_TYPE_CUBETEXTURE
	};

	struct Attachment
	{
		GLuint ID;
		AttachmentType Type;

		Attachment()
			: ID(0), Type(GL_ATTACH_TYPE_RENDERBUFFER)
		{
		}
	};

private:
	GLuint		fboID;
	GLuint		sizex;
	GLuint		sizey;
	Attachment	rendertargets[8];
	Attachment	depthstencil;

public:
	OpenGLFramebuffer(GLuint width, GLuint height);
	~OpenGLFramebuffer();

	bool AttachRenderbuffer(GLenum target, OpenGLFormat format, GLsizei samples = 1);
	bool AttachCubeTexture(GLenum target, OpenGLFormat format, GLenum filter = GL_NEAREST);
	bool AttachTexture(GLenum target, OpenGLFormat format, GLenum filter = GL_NEAREST);
	bool Validate();

	void Attach(GLenum target, GLuint tex, GLint level);
	void Attach(GLenum target, GLuint tex, GLint face, GLint level);
	void CopyDepthStencil(OpenGLFramebuffer* to, GLbitfield mask);
	void Detach(GLenum target);
	void Resolve(OpenGLFramebuffer* to, GLbitfield mask);
	void Set();
	void Unset();

	inline GLuint GetFramebuffer() const				{ return fboID; }
	inline GLuint GetColorAttachment(int index) const	{ return rendertargets[index].ID; }
	inline GLuint GetDepthAttachment() const			{ return depthstencil.ID; }
	inline GLuint GetWidth() const						{ return sizex; }
	inline GLuint GetHeight() const						{ return sizey; }
};

/**
 * \brief Makes 2D rendering easier.
 */
class OpenGLScreenQuad
{
private:
	OpenGLEffect* effect;
	GLuint vertexlayout;

public:
	OpenGLScreenQuad(const char* psfile = "../../Media/ShadersGL/screenquad.frag", const char* defines = nullptr);
	~OpenGLScreenQuad();

	void Draw(bool useinternaleffect = true);
	void SetTextureMatrix(const Math::Matrix& m);

	inline OpenGLEffect* GetEffect()	{ return effect; }
};

// --- Functions --------------------------------------------------------------

bool GLCreateCubeTextureFromDDS(const char* file, bool srgb, GLuint* out);
bool GLCreateCubeTextureFromFiles(const char* files[6], bool srgb, GLuint* out);
bool GLCreateMesh(GLuint numvertices, GLuint numindices, GLuint options, OpenGLVertexElement* decl, OpenGLMesh** mesh);
bool GLCreateMeshFromQM(const char* file, OpenGLMesh** mesh, uint32_t options = 0);
bool GLCreateNormalizationCubemap(GLuint* out);
bool GLCreateTexture(GLsizei width, GLsizei height, GLint miplevels, OpenGLFormat format, GLuint* out, void* data = 0);
bool GLCreateTextureFromDDS(const char* file, bool srgb, GLuint* out);
bool GLCreateTextureFromFile(const char* file, bool srgb, GLuint* out, GLuint flags = 0);
bool GLCreateTextureArrayFromFiles(const std::string* files, uint32_t numfiles, bool srgb, GLuint* out);
bool GLCreateVolumeTextureFromFile(const char* file, bool srgb, GLuint* out);

OpenGLMesh* GLCreateDebugBox();

//GLuint GLCompileShaderFromMemory(GLenum type, const char* code, const char* defines);
GLuint GLCompileShaderFromFile(GLenum type, const char* file, const char* defines);

bool GLCreateEffectFromFile(const char* vsfile, const char* tcsfile, const char* tesfile, const char* gsfile, const char* psfile, OpenGLEffect** effect, const char* defines = nullptr);
//bool GLCreateEffectFromMemory(const char* vscode, const char* tcscode, const char* tescode, const char* gscode, const char* pscode, OpenGLEffect** effect, const char* defines = nullptr);
bool GLCreateComputeProgramFromFile(const char* csfile, OpenGLEffect** effect, const char* defines = nullptr);

void GLRenderText(const std::string& str, GLuint tex, GLsizei width, GLsizei height);
void GLRenderTextEx(const std::string& str, GLuint tex, GLsizei width, GLsizei height, const WCHAR* face, bool border, int style, float emsize);

void GLSetTexture(GLenum unit, GLenum target, GLuint texture);

#endif
