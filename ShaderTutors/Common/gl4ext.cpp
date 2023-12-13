
#include "gl4ext.h"
#include "geometryutils.h"
#include "dds.h"

#include <iostream>
#include <algorithm>

#ifdef _WIN32
// NOTE: include after gl4ext.h
#	include <gdiplus.h>
#elif defined(__APPLE__)
#	import <Cocoa/Cocoa.h>
#	import <CoreText/CoreText.h>
#	import <CoreGraphics/CoreGraphics.h>
#endif

GLint map_Format_Internal[] = {
	0,
	GL_R8,
	GL_RG8,
	GL_RGB8,
	GL_SRGB8,
	GL_RGB8,
	GL_SRGB8,
	GL_RGBA8,
	GL_SRGB8_ALPHA8,
	GL_RGBA8,
	GL_SRGB8_ALPHA8,
	GL_R16UI,

	GL_DEPTH24_STENCIL8,
	GL_DEPTH_COMPONENT32F,

	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,

	GL_R16F,
	GL_RG16F,
	GL_RGBA16F_ARB,
	GL_R32F,
	GL_RG32F,
	GL_RGBA32F_ARB
};

GLenum map_Format_Format[] = {
	0,
	GL_RED,
	GL_RG,
	GL_RGB,
	GL_RGB,
	GL_BGR,
	GL_BGR,
	GL_RGBA,
	GL_RGBA,
	GL_BGRA,
	GL_BGRA,
	GL_RED_INTEGER,

	GL_DEPTH_STENCIL,
	GL_DEPTH_COMPONENT,

	GL_RGBA,
	GL_RGBA,
	GL_RGBA,
	GL_RGBA,

	GL_RED,
	GL_RG,
	GL_RGBA,
	GL_RED,
	GL_RG,
	GL_RGBA
};

GLenum map_Format_Type[] = {
	0,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_SHORT,

	GL_UNSIGNED_INT_24_8,
	GL_FLOAT,

	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,

	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT
};

#ifdef _WIN32
static Gdiplus::Bitmap* Win32LoadPicture(const std::string& file)
{
	std::wstring wstr;
	int size = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), 0, 0);

	wstr.resize(size);
	MultiByteToWideChar(CP_UTF8, 0, file.c_str(), (int)file.size(), &wstr[0], size);

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wstr.c_str(), FALSE);

	if (bitmap->GetLastStatus() != Gdiplus::Ok) {
		delete bitmap;
		bitmap = 0;
	}

	return bitmap;
}
#elif defined(__APPLE__)
std::string GetResource(const std::string& file)
{
	std::string name, ext;
	size_t loc1 = file.find_last_of('.');
	size_t loc2 = file.find_last_of('/');
	
	assert(loc1 != std::string::npos);
	
	if (loc2 != std::string::npos)
		name = file.substr(loc2 + 1, loc1 - loc2 - 1);
	else
		name = file.substr(0, loc1);
	
	ext = file.substr(loc1 + 1);
	
	NSString* path = [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:name.c_str()] ofType:[NSString stringWithUTF8String:ext.c_str()]];
	return std::string([path UTF8String]);
}
#endif

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

template <int size>
static char* CopyString(char (&dst)[size], const char* src)
{
#ifdef _MSC_VER
	strcpy_s(dst, size, src);
	return dst;
#else
	return strcpy(dst, src);
#endif
}

// --- Structures impl --------------------------------------------------------

OpenGLMaterial::OpenGLMaterial()
{
	Texture = 0;
	NormalMap = 0;
}

OpenGLMaterial::~OpenGLMaterial()
{
	if (Texture != 0)
		glDeleteTextures(1, &Texture);

	if (NormalMap != 0)
		glDeleteTextures(1, &NormalMap);

	OpenGLContentManager().UnregisterTexture(Texture);
	OpenGLContentManager().UnregisterTexture(NormalMap);
}

// --- OpenGLContentRegistry impl ---------------------------------------------

OpenGLContentRegistry* OpenGLContentRegistry::_inst = nullptr;

OpenGLContentRegistry& OpenGLContentRegistry::Instance()
{
	if (!_inst)
		_inst = new OpenGLContentRegistry();

	return *_inst;
}

void OpenGLContentRegistry::Release()
{
	if (_inst)
		delete _inst;

	_inst = nullptr;
}

OpenGLContentRegistry::OpenGLContentRegistry()
{
}

OpenGLContentRegistry::~OpenGLContentRegistry()
{
}

void OpenGLContentRegistry::RegisterTexture(const std::string& file, GLuint tex)
{
	std::string name;
	Math::GetFile(name, file);

	GL_ASSERT(textures.count(name) == 0);

	if (tex != 0)
		textures.insert(TextureMap::value_type(name, tex));
}

void OpenGLContentRegistry::UnregisterTexture(GLuint tex)
{
	for (auto it = textures.begin(); it != textures.end(); ++it) {
		if (it->second == tex) {
			textures.erase(it);
			break;
		}
	}
}

GLuint OpenGLContentRegistry::IDTexture(const std::string& file)
{
	std::string name;
	Math::GetFile(name, file);

	TextureMap::iterator it = textures.find(name);

	if (it == textures.end())
		return 0;

	return it->second;
}

// --- OpenGLMesh impl --------------------------------------------------------

OpenGLMesh::OpenGLMesh()
{
	numsubsets		= 0;
	numvertices		= 0;
	numindices		= 0;
	subsettable		= nullptr;
	materials		= nullptr;
	vertexbuffer	= 0;
	indexbuffer		= 0;
	vertexlayout	= 0;
	meshoptions		= 0;

	vertexdecl.Elements = 0;
	vertexdecl.Stride = 0;

	vertexdata_locked.ptr = 0;
	indexdata_locked.ptr = 0;
}

OpenGLMesh::~OpenGLMesh()
{
	Destroy();
}

void OpenGLMesh::Destroy()
{
	delete[] vertexdecl.Elements;

	GL_SAFE_DELETE_BUFFER(vertexbuffer);
	GL_SAFE_DELETE_BUFFER(indexbuffer);

	if (vertexlayout != 0) {
		glDeleteVertexArrays(1, &vertexlayout);
		vertexlayout = 0;
	}

	delete[] subsettable;
	delete[] materials;

	subsettable = nullptr;
	materials = nullptr;
	numsubsets = 0;
}

void OpenGLMesh::RecreateVertexLayout()
{
	if (vertexlayout != 0)
		glDeleteVertexArrays(1, &vertexlayout);

	glGenVertexArrays(1, &vertexlayout);
	vertexdecl.Stride = 0;

	// calculate stride
	for (int i = 0; i < 16; ++i) {
		OpenGLVertexElement& elem = vertexdecl.Elements[i];

		if (elem.Stream == 0xff)
			break;

		switch (elem.Type) {
		case GLDECLTYPE_GLCOLOR:
		case GLDECLTYPE_FLOAT1:		vertexdecl.Stride += 4;		break;
		case GLDECLTYPE_FLOAT2:		vertexdecl.Stride += 8;		break;
		case GLDECLTYPE_FLOAT3:		vertexdecl.Stride += 12;	break;
		case GLDECLTYPE_FLOAT4:		vertexdecl.Stride += 16;	break;

		default:
			break;
		}
	}

	glBindVertexArray(vertexlayout);
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

		// bind locations
		for (int i = 0; i < 16; ++i) {
			OpenGLVertexElement& elem = vertexdecl.Elements[i];

			if (elem.Stream == 0xff)
				break;

			glEnableVertexAttribArray(elem.Usage);

			switch (elem.Usage) {
			case GLDECLUSAGE_POSITION:
				glVertexAttribPointer(elem.Usage, (elem.Type == GLDECLTYPE_FLOAT4 ? 4 : 3), GL_FLOAT, GL_FALSE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_COLOR:
				glVertexAttribPointer(elem.Usage, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_NORMAL:
				glVertexAttribPointer(elem.Usage, (elem.Type == GLDECLTYPE_FLOAT4 ? 4 : 3), GL_FLOAT, GL_FALSE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_TEXCOORD:
				// haaack...
				glVertexAttribPointer(elem.Usage + elem.UsageIndex, (elem.Type + 1), GL_FLOAT, GL_FALSE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_TANGENT:
				glVertexAttribPointer(elem.Usage, 3, GL_FLOAT, GL_FALSE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_BINORMAL:
				glVertexAttribPointer(elem.Usage, 3, GL_FLOAT, GL_FALSE, vertexdecl.Stride, (const GLvoid*)elem.Offset);
				break;

			// TODO:

			default:
				std::cout << "Unhandled layout element...\n";
				break;
			}
		}
	}
	glBindVertexArray(0);
}

bool OpenGLMesh::LockVertexBuffer(GLuint offset, GLuint size, GLuint flags, void** data)
{
	if (offset >= numvertices * vertexdecl.Stride) {
		(*data) = nullptr;
		return false;
	}

	if (size == 0)
		size = numvertices * vertexdecl.Stride - offset;

	if (flags == 0)
		flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT;

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	vertexdata_locked.ptr = glMapBufferRange(GL_ARRAY_BUFFER, offset, size, flags);
	vertexdata_locked.flags = flags;

	if (!vertexdata_locked.ptr)
		return false;

	(*data) = vertexdata_locked.ptr;
	return true;
}

bool OpenGLMesh::LockIndexBuffer(GLuint offset, GLuint size, GLuint flags, void** data)
{
	GLuint istride = ((meshoptions & GLMESH_32BIT) ? 4 : 2);

	if (offset >= numindices * istride) {
		(*data) = nullptr;
		return false;
	}

	if (size == 0)
		size = numindices * istride - offset;

	if (flags == 0)
		flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

	indexdata_locked.ptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, offset, size, flags);
	indexdata_locked.flags = flags;

	if (!indexdata_locked.ptr)
		return false;

	(*data) = indexdata_locked.ptr;
	return true;
}

void OpenGLMesh::Draw()
{
	for (GLuint i = 0; i < numsubsets; ++i)
		DrawSubset(i);
}

void OpenGLMesh::DrawInstanced(GLuint numinstances)
{
	for (GLuint i = 0; i < numsubsets; ++i)
		DrawSubsetInstanced(i, numinstances);
}

void OpenGLMesh::DrawSubset(GLuint subset, bool bindtextures)
{
	if (vertexlayout == 0 || numvertices == 0)
		return;

	if (subsettable != nullptr && subset < numsubsets) {
		const OpenGLAttributeRange& attr = subsettable[subset];
		const OpenGLMaterial& mat = materials[subset];

		if (!attr.Enabled)
			return;

		GLenum itype = (meshoptions & GLMESH_32BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		GLuint start = attr.IndexStart * ((meshoptions & GLMESH_32BIT) ? 4 : 2);

		if (bindtextures) {
			if (mat.Texture != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mat.Texture);
			}

			if (mat.NormalMap != 0) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mat.NormalMap);
			}
		}

		glBindVertexArray(vertexlayout);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

		if (attr.IndexCount == 0) {
			glDrawArrays(attr.PrimitiveType, attr.VertexStart, attr.VertexCount);
		} else {
			if (attr.VertexCount == 0)
				glDrawElements(attr.PrimitiveType, attr.IndexCount, itype, (char*)0 + start);
			else
				glDrawRangeElements(attr.PrimitiveType, attr.VertexStart, attr.VertexStart + attr.VertexCount - 1, attr.IndexCount, itype, (char*)0 + start);
		}
	}
}

void OpenGLMesh::DrawSubset(GLuint subset, std::function<void (const OpenGLMaterial&)> callback)
{
	if (vertexlayout == 0 || numvertices == 0)
		return;

	if (subsettable != nullptr && subset < numsubsets) {
		const OpenGLAttributeRange& attr = subsettable[subset];
		const OpenGLMaterial& mat = materials[subset];

		if (!attr.Enabled)
			return;

		GLenum itype = (meshoptions & GLMESH_32BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		GLuint start = attr.IndexStart * ((meshoptions & GLMESH_32BIT) ? 4 : 2);

		if (callback)
			callback(mat);

		if (mat.Texture != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mat.Texture);
		}

		if (mat.NormalMap != 0) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mat.NormalMap);
		}

		glBindVertexArray(vertexlayout);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

		if (attr.IndexCount == 0) {
			glDrawArrays(attr.PrimitiveType, attr.VertexStart, attr.VertexCount);
		} else {
			if (attr.VertexCount == 0)
				glDrawElements(attr.PrimitiveType, attr.IndexCount, itype, (char*)0 + start);
			else
				glDrawRangeElements(attr.PrimitiveType, attr.VertexStart, attr.VertexStart + attr.VertexCount - 1, attr.IndexCount, itype, (char*)0 + start);
		}
	}
}

void OpenGLMesh::DrawSubsetInstanced(GLuint subset, GLuint numinstances, bool bindtextures)
{
	// NOTE: use SSBO, dummy... (or modify this class)

	if (vertexlayout == 0 || numvertices == 0 || numinstances == 0)
		return;

	if (subsettable != nullptr && subset < numsubsets) {
		const OpenGLAttributeRange& attr = subsettable[subset];
		const OpenGLMaterial& mat = materials[subset];

		if (!attr.Enabled)
			return;

		GLenum itype = (meshoptions & GLMESH_32BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
		GLuint start = attr.IndexStart * ((meshoptions & GLMESH_32BIT) ? 4 : 2);

		if (bindtextures) {
			if (mat.Texture != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mat.Texture);
			}

			if (mat.NormalMap != 0) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mat.NormalMap);
			}
		}

		glBindVertexArray(vertexlayout);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

		if (attr.IndexCount == 0)
			glDrawArraysInstanced(attr.PrimitiveType, attr.VertexStart, attr.VertexCount, numinstances);
		else
			glDrawElementsInstanced(attr.PrimitiveType, attr.IndexCount, itype, (char*)0 + start, numinstances);
	}
}

void OpenGLMesh::EnableSubset(GLuint subset, bool enable)
{
	if (subsettable != nullptr && subset < numsubsets)
		subsettable[subset].Enabled = enable;
}

void OpenGLMesh::CalculateBoundingBox()
{
	GL_ASSERT(vertexbuffer != 0);

	void* vdata = nullptr;
	GLushort offset = 0xffff;

	for (int i = 0; ; ++i) {
		const OpenGLVertexElement& elem = vertexdecl.Elements[i];

		if (elem.Stream == 0xff)
			break;

		if (elem.Usage == GLDECLUSAGE_POSITION) {
			GL_ASSERT(elem.Type == GLDECLTYPE_FLOAT3);
			offset = elem.Offset;

			break;
		}
	}

	GL_ASSERT(offset != 0xffff);
	GL_ASSERT(LockVertexBuffer(0, 0, GLLOCK_READONLY, &vdata));

	boundingbox = Math::AABox();

	for (GLuint i = 0; i < numvertices; ++i) {
		Math::Vector3* pos = (Math::Vector3*)((char*)vdata + i * vertexdecl.Stride + offset);
		boundingbox.Add(pos->x, pos->y, pos->z);
	}

	UnlockVertexBuffer();
}

void OpenGLMesh::GenerateTangentFrame()
{
	GL_ASSERT(vertexdecl.Stride == sizeof(GeometryUtils::CommonVertex));
	GL_ASSERT(vertexbuffer != 0);

	GeometryUtils::CommonVertex*	oldvdata	= nullptr;
	GeometryUtils::TBNVertex*		newvdata	= nullptr;
	void*							idata		= nullptr;
	GLuint							newbuffer	= 0;
	uint32_t						i1, i2, i3;
	bool							is32bit		= ((meshoptions & GLMESH_32BIT) == GLMESH_32BIT);

	GL_ASSERT(LockVertexBuffer(0, 0, GLLOCK_READONLY, (void**)&oldvdata));
	GL_ASSERT(LockIndexBuffer(0, 0, GLLOCK_READONLY, (void**)&idata));

	newvdata = new GeometryUtils::TBNVertex[numvertices];

	for (GLuint i = 0; i < numsubsets; ++i) {
		const OpenGLAttributeRange& subset = subsettable[i];
		GL_ASSERT(subset.IndexCount > 0);

		GeometryUtils::CommonVertex*	oldsubsetdata	= (oldvdata + subset.VertexStart);
		GeometryUtils::TBNVertex*		newsubsetdata	= (newvdata + subset.VertexStart);
		void*							subsetidata		= ((uint16_t*)idata + subset.IndexStart);

		if (is32bit)
			subsetidata = ((uint32_t*)idata + subset.IndexStart);

		// initialize new data
		for (uint32_t j = 0; j < subset.VertexCount; ++j) {
			GeometryUtils::CommonVertex& oldvert = oldsubsetdata[j];
			GeometryUtils::TBNVertex& newvert = newsubsetdata[j];

			newvert.x = oldvert.x;
			newvert.y = oldvert.y;
			newvert.z = oldvert.z;

			newvert.nx = oldvert.nx;
			newvert.ny = oldvert.ny;
			newvert.nz = oldvert.nz;

			newvert.u = oldvert.u;
			newvert.v = oldvert.v;

			newvert.tx = newvert.bx = 0;
			newvert.ty = newvert.by = 0;
			newvert.tz = newvert.bz = 0;
		}

		for (uint32_t j = 0; j < subset.IndexCount; j += 3) {
			if (is32bit) {
				i1 = *((uint32_t*)subsetidata + j + 0) - subset.VertexStart;
				i2 = *((uint32_t*)subsetidata + j + 1) - subset.VertexStart;
				i3 = *((uint32_t*)subsetidata + j + 2) - subset.VertexStart;
			} else {
				i1 = *((uint16_t*)subsetidata + j + 0) - subset.VertexStart;
				i2 = *((uint16_t*)subsetidata + j + 1) - subset.VertexStart;
				i3 = *((uint16_t*)subsetidata + j + 2) - subset.VertexStart;
			}

			GeometryUtils::AccumulateTangentFrame(newsubsetdata, i1, i2, i3);
		}

		for (uint32_t j = 0; j < subset.VertexCount; ++j) {
			GeometryUtils::OrthogonalizeTangentFrame(newsubsetdata[j]);
		}
	}

	UnlockIndexBuffer();
	UnlockVertexBuffer();

	glDeleteBuffers(1, &vertexbuffer);
	glGenBuffers(1, &vertexbuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, numvertices * sizeof(GeometryUtils::TBNVertex), newvdata, ((meshoptions & GLMESH_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete[] newvdata;

	OpenGLVertexElement* olddecl = vertexdecl.Elements;
	int numdeclelems = 0;

	for (size_t i = 0; i < 16; ++i) {
		const OpenGLVertexElement& elem = vertexdecl.Elements[i];

		if (elem.Stream == 0xff)
			break;

		++numdeclelems;
	}

	vertexdecl.Elements = new OpenGLVertexElement[numdeclelems + 3];
	memcpy (vertexdecl.Elements, olddecl, numdeclelems * sizeof(OpenGLVertexElement));

	vertexdecl.Elements[numdeclelems + 0].Stream = 0;
	vertexdecl.Elements[numdeclelems + 0].Offset = 32;
	vertexdecl.Elements[numdeclelems + 0].Type = GLDECLTYPE_FLOAT3;
	vertexdecl.Elements[numdeclelems + 0].Usage = GLDECLUSAGE_TANGENT;
	vertexdecl.Elements[numdeclelems + 0].UsageIndex = 0;

	vertexdecl.Elements[numdeclelems + 1].Stream = 0;
	vertexdecl.Elements[numdeclelems + 1].Offset = 44;
	vertexdecl.Elements[numdeclelems + 1].Type = GLDECLTYPE_FLOAT3;
	vertexdecl.Elements[numdeclelems + 1].Usage = GLDECLUSAGE_BINORMAL;
	vertexdecl.Elements[numdeclelems + 1].UsageIndex = 0;

	vertexdecl.Elements[numdeclelems + 2].Stream = 0xff;

	delete[] olddecl;
	RecreateVertexLayout();

	GL_ASSERT(vertexdecl.Stride == sizeof(GeometryUtils::TBNVertex));
}

void OpenGLMesh::ReorderSubsets(GLuint newindices[])
{
	OpenGLAttributeRange tmp;

	for (GLuint i = 0; i < numsubsets; ++i) {
		tmp = subsettable[i];
		subsettable[i] = subsettable[newindices[i]];
		subsettable[newindices[i]] = tmp;

		for (GLuint j = i; j < numsubsets; ++j) {
			if (newindices[j] == i) {
				newindices[j] = newindices[i];
				break;
			}
		}
	}
}

void OpenGLMesh::UnlockVertexBuffer()
{
	if (vertexdata_locked.ptr != nullptr && vertexbuffer != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);

		vertexdata_locked.ptr = nullptr;
	}
}

void OpenGLMesh::UnlockIndexBuffer()
{
	if (indexdata_locked.ptr != nullptr && indexbuffer != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

		indexdata_locked.ptr = nullptr;
	}
}

void OpenGLMesh::SetAttributeTable(const OpenGLAttributeRange* table, GLuint size)
{
	delete[] subsettable;

	subsettable = new OpenGLAttributeRange[size];
	memcpy(subsettable, table, size * sizeof(OpenGLAttributeRange));

	numsubsets = size;
}

// --- OpenGLEffect impl ------------------------------------------------------

OpenGLEffect::OpenGLEffect()
{
	floatvalues	= nullptr;
	intvalues	= nullptr;
	uintvalues	= nullptr;

	floatcap	= 0;
	floatsize	= 0;
	intcap		= 0;
	intsize		= 0;
	uintcap		= 0;
	uintsize	= 0;
	program		= 0;
}

OpenGLEffect::~OpenGLEffect()
{
	Destroy();
}

void OpenGLEffect::Destroy()
{
	if (floatvalues != nullptr)
		delete[] floatvalues;

	if (intvalues != nullptr)
		delete[] intvalues;

	if (uintvalues != nullptr)
		delete[] uintvalues;

	floatvalues = nullptr;
	intvalues = nullptr;
	uintvalues = nullptr;

	if (program)
		glDeleteProgram(program);

	program = 0;
}

void OpenGLEffect::AddUniform(const char* name, GLuint location, GLuint count, GLenum type)
{
	Uniform uni;

	if (strlen(name) >= sizeof(uni.Name))
		throw std::length_error("Uniform name too long");

	CopyString(uni.Name, name);

	if (type == GL_FLOAT_MAT4)
		count = 4;

	uni.Type = type;
	uni.RegisterCount = count;
	uni.Location = location;
	uni.Changed = true;

	if (type == GL_FLOAT || (type >= GL_FLOAT_VEC2 && type <= GL_FLOAT_VEC4) || type == GL_FLOAT_MAT4) {
		uni.StartRegister = floatsize;

		if (floatsize + count > floatcap) {
			uint32_t newcap = std::max<uint32_t>(floatsize + count, floatsize + 8);

			floatvalues = (float*)realloc(floatvalues, newcap * 4 * sizeof(float));
			floatcap = newcap;
		}

		float* reg = (floatvalues + uni.StartRegister * 4);

		if (type == GL_FLOAT_MAT4)
			Math::MatrixIdentity((Math::Matrix&)*reg);
		else
			memset(reg, 0, uni.RegisterCount * 4 * sizeof(float));

		floatsize += count;
	} else if (
		type == GL_INT ||
		(type >= GL_INT_VEC2 && type <= GL_INT_VEC4) ||
		type == GL_SAMPLER_2D || type == GL_SAMPLER_2D_ARRAY || type == GL_SAMPLER_BUFFER ||
		type == GL_SAMPLER_CUBE || type == GL_IMAGE_2D || type == GL_UNSIGNED_INT_IMAGE_2D)
	{
		uni.StartRegister = intsize;

		if (intsize + count > intcap) {
			uint32_t newcap = std::max<uint32_t>(intsize + count, intsize + 8);

			intvalues = (int*)realloc(intvalues, newcap * 4 * sizeof(int));
			intcap = newcap;
		}

		int* reg = (intvalues + uni.StartRegister * 4);
		memset(reg, 0, uni.RegisterCount * 4 * sizeof(int));

		intsize += count;
	} else if (type == GL_UNSIGNED_INT_VEC4) {
		uni.StartRegister = uintsize;

		if (uintsize + count > uintcap) {
			uint32_t newcap = std::max<uint32_t>(uintsize + count, uintsize + 8);

			uintvalues = (GLuint*)realloc(uintvalues, newcap * 4 * sizeof(GLuint));
			uintcap = newcap;
		}

		GLuint* reg = (uintvalues + uni.StartRegister * 4);
		memset(reg, 0, uni.RegisterCount * 4 * sizeof(GLuint));

		uintsize += count;
	} else {
		// not handled
		throw std::invalid_argument("This uniform type is not supported");
	}

	uniforms.Insert(uni);
}

void OpenGLEffect::AddUniformBlock(const char* name, GLint index, GLint binding, GLint blocksize)
{
	UniformBlock block;

	if (strlen(name) >= sizeof(block.Name))
		throw std::length_error("Uniform block name too long");

	block.Index = index;
	block.Binding = binding;
	block.BlockSize = blocksize;

	CopyString(block.Name, name);
	uniformblocks.Insert(block);
}

void OpenGLEffect::BindAttributes()
{
	typedef std::map<std::string, GLuint> SemanticMap;

	if (program == 0)
		return;

	SemanticMap	attribmap;
	GLint		count;
	GLenum		type;
	GLint		size, loc;
	GLchar		attribname[256];

	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);

	attribmap["my_Position"]	= GLDECLUSAGE_POSITION;
	attribmap["my_Normal"]		= GLDECLUSAGE_NORMAL;
	attribmap["my_Tangent"]		= GLDECLUSAGE_TANGENT;
	attribmap["my_Binormal"]	= GLDECLUSAGE_BINORMAL;
	attribmap["my_Color"]		= GLDECLUSAGE_COLOR;
	attribmap["my_Texcoord0"]	= GLDECLUSAGE_TEXCOORD;
	attribmap["my_Texcoord1"]	= GLDECLUSAGE_TEXCOORD + 10;
	attribmap["my_Texcoord2"]	= GLDECLUSAGE_TEXCOORD + 11;
	attribmap["my_Texcoord3"]	= GLDECLUSAGE_TEXCOORD + 12;
	attribmap["my_Texcoord4"]	= GLDECLUSAGE_TEXCOORD + 13;
	attribmap["my_Texcoord5"]	= GLDECLUSAGE_TEXCOORD + 14;
	attribmap["my_Texcoord6"]	= GLDECLUSAGE_TEXCOORD + 15;
	attribmap["my_Texcoord7"]	= GLDECLUSAGE_TEXCOORD + 16;

	for (int i = 0; i < count; ++i) {
		memset(attribname, 0, sizeof(attribname));
		glGetActiveAttrib(program, i, 256, NULL, &size, &type, attribname);

		if (attribname[0] == 'g' && attribname[1] == 'l')
			continue;

		loc = glGetAttribLocation(program, attribname);

		auto it = attribmap.find(attribname);

		if (loc == -1 || it == attribmap.end())
			std::cout << "Invalid attribute found. Use the my_<semantic> syntax!\n";
		else
			glBindAttribLocation(program, it->second, attribname);
	}

	// bind fragment shader output
	GLint numrts = 0;

	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &numrts);
	glBindFragDataLocation(program, 0, "my_FragColor0");

	if (numrts > 1)
		glBindFragDataLocation(program, 1, "my_FragColor1");

	if (numrts > 2)
		glBindFragDataLocation(program, 2, "my_FragColor2");
		
	if (numrts > 3)
		glBindFragDataLocation(program, 3, "my_FragColor3");

	// relink
	glLinkProgram(program);
}

void OpenGLEffect::QueryUniforms()
{
	GLint		count;
	GLenum		type;
	GLsizei		length;
	GLint		size, loc;
	GLchar		uniname[256];

	if (program == 0)
		return;

	memset(uniname, 0, sizeof(uniname));
	uniforms.Clear();

	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &count);

	if (count > 0) {
		// uniform blocks
		std::vector<std::string> blocknames;
		blocknames.reserve(count);

		for (int i = 0; i < count; ++i) {
			GLsizei namelength = 0;
			char name[32];

			glGetActiveUniformBlockName(program, i, 32, &namelength, name);

			name[namelength] = 0;
			blocknames.push_back(name);
		}

		for (size_t i = 0; i < blocknames.size(); ++i) {
			GLint blocksize = 0;
			GLint index = glGetUniformBlockIndex(program, blocknames[i].c_str());

			glGetActiveUniformBlockiv (program, (GLuint)i, GL_UNIFORM_BLOCK_DATA_SIZE, &blocksize);
			AddUniformBlock(blocknames[i].c_str(), index, INT_MAX, blocksize);
		}
	} else {
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

		// uniforms
		for (int i = 0; i < count; ++i) {
			memset(uniname, 0, sizeof(uniname));

			glGetActiveUniform(program, i, 256, &length, &size, &type, uniname);
			loc = glGetUniformLocation(program, uniname);

			for (int j = 0; j < length; ++j) {
				if (uniname[j] == '[')
					uniname[j] = '\0';
			}

			if (loc == -1 || type == 0x1405)	// not specificed in standard, subroutine
				continue;

			AddUniform(uniname, loc, size, type);
		}
	}
}

void OpenGLEffect::Introspect()
{
#ifndef __APPLE__
	GLenum props1[] = { GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
	GLenum props2[] = { GL_ACTIVE_VARIABLES };
	GLenum props3[] = { GL_OFFSET };

	GLint count = 0;
	GLint varcount = 0;
	GLint length = 0;
	GLint values[10] = {};

	glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count);

	for (GLint i = 0; i < count; ++i) {
		glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, i, ARRAY_SIZE(props1), props1, ARRAY_SIZE(values), &length, values);

		varcount = values[1];
		std::cout << "Shader storage block (" << i << "): size = " << values[0] << " variables = " << varcount << "\n";

		glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, i, ARRAY_SIZE(props2), props2, ARRAY_SIZE(values), &length, values);

		for (GLint j = 0; j < varcount; ++j) {
			glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, values[j], ARRAY_SIZE(props3), props3, ARRAY_SIZE(values), &length, values);
			std::cout << "  variable (" << j << "): offset = " << values[0] << "\n";
		}
	}

	glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);

	GLenum props4[] = { GL_TYPE, GL_NAME_LENGTH };
	std::string name;

	for (GLint i = 0; i < count; ++i) {
		glGetProgramResourceiv(program, GL_UNIFORM, i, ARRAY_SIZE(props4), props4, ARRAY_SIZE(values), &length, values);

		if (values[0] == GL_IMAGE_2D || values[0] == GL_UNSIGNED_INT_IMAGE_2D) {
			name.resize(values[1]);

			glGetProgramResourceName(program, GL_UNIFORM, i, (GLsizei)name.size(), NULL, &name[0]);
			name.pop_back();

			std::cout << "Image (" << name << "): \n";

			//glGetUniformLocation(program, name);
		}
	}
#endif
}

void OpenGLEffect::Begin()
{
	glUseProgram(program);
	CommitChanges();
}

void OpenGLEffect::CommitChanges()
{
	for (size_t i = 0; i < uniforms.Size(); ++i) {
		const Uniform& uni = uniforms[i];

		float* floatdata = (floatvalues + uni.StartRegister * 4);
		int* intdata = (intvalues + uni.StartRegister * 4);
		GLuint* uintdata = (uintvalues + uni.StartRegister * 4);

		if (!uni.Changed)
			continue;

		uni.Changed = false;

		switch (uni.Type) {
		case GL_FLOAT:
			glUniform1fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC2:
			glUniform2fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC3:
			glUniform3fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC4:
			glUniform4fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_MAT4:
			glUniformMatrix4fv(uni.Location, uni.RegisterCount / 4, false, floatdata);
			break;

		case GL_UNSIGNED_INT_VEC4:
			glUniform4uiv(uni.Location, uni.RegisterCount, uintdata);
			break;

		case GL_INT:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_BUFFER:
		case GL_IMAGE_2D:
		case GL_UNSIGNED_INT_IMAGE_2D:
			glUniform1i(uni.Location, intdata[0]);
			break;

		default:
			break;
		}
	}
}

void OpenGLEffect::End()
{
	// do nothing
}

void OpenGLEffect::SetMatrix(const char* name, const float* value)
{
	SetVector(name, value);
}

void OpenGLEffect::SetVector(const char* name, const float* value)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		memcpy(reg, value, uni.RegisterCount * 4 * sizeof(float));
		uni.Changed = true;
	}
}

void OpenGLEffect::SetVectorArray(const char* name, const float* values, GLsizei count)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		if (count > uni.RegisterCount)
			count = uni.RegisterCount;

		memcpy(reg, values, count * 4 * sizeof(float));
		uni.Changed = true;
	}
}

void OpenGLEffect::SetFloat(const char* name, float value)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		reg[0] = value;
		uni.Changed = true;
	}
}

void OpenGLEffect::SetFloatArray(const char* name, const float* values, GLsizei count)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		if (count > uni.RegisterCount)
			count = uni.RegisterCount;

		memcpy(reg, values, count * sizeof(float));
		uni.Changed = true;
	}
}

void OpenGLEffect::SetInt(const char* name, int value)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		int* reg = (intvalues + uni.StartRegister * 4);

		reg[0] = value;
		uni.Changed = true;
	}
}

void OpenGLEffect::SetUIntVector(const char* name, const GLuint* value)
{
	Uniform test;
	CopyString(test.Name, name);

	size_t id = uniforms.Find(test);

	if (id < uniforms.Size()) {
		const Uniform& uni = uniforms[id];
		GLuint* reg = (uintvalues + uni.StartRegister * 4);

		memcpy(reg, value, uni.RegisterCount * 4 * sizeof(GLuint));
		uni.Changed = true;
	}
}

void OpenGLEffect::SetUniformBlockBinding(const char* name, GLint binding)
{
	UniformBlock temp;
	CopyString(temp.Name, name);

	size_t id = uniformblocks.Find(temp);

	if (id < uniformblocks.Size()) {
		const UniformBlock& block = uniformblocks[id];

		block.Binding = binding;
		glUniformBlockBinding(program, block.Index, binding);
	}
}

// --- OpenGLProgramPipeline impl ---------------------------------------------

OpenGLProgramPipeline::OpenGLProgramPipeline()
{
	pipeline = 0;
	program = 0;

	glGenProgramPipelines(1, &pipeline);
	memset(shaders, 0, sizeof(shaders));
}

OpenGLProgramPipeline::~OpenGLProgramPipeline()
{
	if (program != 0)
		glDeleteProgram(program);

	if (pipeline != 0)
		glDeleteProgramPipelines(1, &pipeline);

	pipeline = 0;
	program = 0;
}

bool OpenGLProgramPipeline::AddShader(GLenum type, const char* file, const char* defines)
{
	int index = -1;

	switch (type) {
	case GL_VERTEX_SHADER:			index = 0;	break;
	case GL_TESS_CONTROL_SHADER:	index = 1;	break;
	case GL_TESS_EVALUATION_SHADER:	index = 2;	break;
	case GL_GEOMETRY_SHADER:		index = 3;	break;
	case GL_FRAGMENT_SHADER:		index = 4;	break;

	default:
		return false;
	}

	if (shaders[index] != 0)
		return false;

	shaders[index] = GLCompileShaderFromFile(type, file, defines);

	if (shaders[index] == 0)
		return false;

	return true;
}

bool OpenGLProgramPipeline::Assemble()
{
	GLbitfield pipelinestages[] = {
		GL_VERTEX_SHADER_BIT,
		GL_TESS_CONTROL_SHADER_BIT,
		GL_TESS_EVALUATION_SHADER_BIT,
		GL_GEOMETRY_SHADER_BIT,
		GL_FRAGMENT_SHADER_BIT
	};

	if (program != 0)
		return false;

	char log[1024];
	GLint success = GL_FALSE;
	GLint length = 0;
	GLbitfield stages = 0;

	program = glCreateProgram();

	for (int i = 0; i < 5; ++i) {
		if (shaders[i] != 0) {
			glAttachShader(program, shaders[i]);
			stages |= pipelinestages[i];
		}
	}

	glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (success != GL_TRUE) {
		glGetProgramInfoLog(program, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";
		return false;
	}

	// delete shaders
	for (int i = 0; i < 5; ++i) {
		if (shaders[i] != 0) {
			glDetachShader(program, shaders[i]);
			glDeleteShader(shaders[i]);

			shaders[i] = 0;
		}
	}

	// add to pipeline
	glUseProgramStages(pipeline, stages, program);

	glValidateProgramPipeline(pipeline);
	glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &success);

	if (success != GL_TRUE) {
		glGetProgramPipelineInfoLog(pipeline, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";
		return false;
	}

	return true;
}

void OpenGLProgramPipeline::Bind()
{
	glBindProgramPipeline(pipeline);
}

void OpenGLProgramPipeline::Introspect()
{
#ifndef __APPLE__
	std::string name;
	GLenum props1[] = { GL_LOCATION, GL_NAME_LENGTH };

	GLint count = 0;
	GLint namelength = 0;
	GLint length = 0;
	GLint values[10] = {};

	glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);

	for (GLint i = 0; i < count; ++i) {
		glGetProgramResourceiv(program, GL_UNIFORM, i, ARRAY_SIZE(props1), props1, ARRAY_SIZE(values), &length, values);

		namelength = values[1];
		name.resize(namelength);

		glGetProgramResourceName(program, GL_UNIFORM, i, namelength, &length, &name[0]);
		name.pop_back();

		std::cout << "Uniform (" << name << "): location = " << values[0] << "\n";
	}
#endif
}

void OpenGLProgramPipeline::UseProgramStages(OpenGLProgramPipeline* other, GLbitfield stages)
{
	if (other->program == 0)
		return;

	glUseProgramStages(pipeline, stages, other->program);
}

// --- OpenGLFramebuffer impl -------------------------------------------------

OpenGLFramebuffer::OpenGLFramebuffer(GLuint width, GLuint height)
{
	glGenFramebuffers(1, &fboID);

	sizex = width;
	sizey = height;
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	
	for (int i = 0; i < 8; ++i) {
		if (rendertargets[i].ID != 0) {
			if (rendertargets[i].Type == GL_ATTACH_TYPE_RENDERBUFFER) {
				glDeleteRenderbuffers(1, &rendertargets[i].ID);
			} else {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
				glDeleteTextures(1, &rendertargets[i].ID);
			}
		}
	}

	if (depthstencil.ID != 0) {
		if (depthstencil.Type == GL_ATTACH_TYPE_RENDERBUFFER) {
			glDeleteRenderbuffers(1, &depthstencil.ID);
		} else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			glDeleteTextures(1, &depthstencil.ID);
		}
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fboID);
}

bool OpenGLFramebuffer::AttachRenderbuffer(GLenum target, OpenGLFormat format, GLsizei samples)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT) {
		attach = &depthstencil;
	} else if (target >= GL_COLOR_ATTACHMENT0 && target < GL_COLOR_ATTACHMENT8) {
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];
	} else {
		std::cout << "Target is invalid!\n";
		return false;
	}

	if (attach->ID != 0) {
		std::cout << "Already attached to this target!\n";
		return false;
	}

	glGenRenderbuffers(1, &attach->ID);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glBindRenderbuffer(GL_RENDERBUFFER, attach->ID);

	if (samples > 1)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, map_Format_Internal[format], sizex, sizey);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, map_Format_Internal[format], sizex, sizey);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, GL_RENDERBUFFER, attach->ID);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	attach->Type = GL_ATTACH_TYPE_RENDERBUFFER;

	return true;
}

bool OpenGLFramebuffer::AttachCubeTexture(GLenum target, OpenGLFormat format, GLenum filter)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT) {
		attach = &depthstencil;
	} else if (target >= GL_COLOR_ATTACHMENT0 && target < GL_COLOR_ATTACHMENT8) {
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];
	} else {
		std::cout << "Target is invalid!\n";
		return false;
	}

	if (attach->ID != 0) {
		std::cout << "Already attached to this target!\n";
		return false;
	}

	glGenTextures(1, &attach->ID);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, attach->ID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filter);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, map_Format_Internal[format], sizex, sizex, 0,
		map_Format_Format[format], map_Format_Type[format], NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_CUBE_MAP_POSITIVE_X, attach->ID, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	attach->Type = GL_ATTACH_TYPE_CUBETEXTURE;

	return true;
}

bool OpenGLFramebuffer::AttachTexture(GLenum target, OpenGLFormat format, GLenum filter)
{
	Attachment* attach = 0;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT) {
		attach = &depthstencil;
	} else if (target >= GL_COLOR_ATTACHMENT0 && target < GL_COLOR_ATTACHMENT8) {
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];
	} else {
		std::cout << "Target is invalid!\n";
		return false;
	}

	if (attach->ID != 0) {
		std::cout << "Already attached to this target!\n";
		return false;
	}

	glGenTextures(1, &attach->ID);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glBindTexture(GL_TEXTURE_2D, attach->ID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	glTexImage2D(GL_TEXTURE_2D, 0, map_Format_Internal[format], sizex, sizey, 0, map_Format_Format[format], map_Format_Type[format], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, attach->ID, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	attach->Type = GL_ATTACH_TYPE_TEXTURE;

	return true;
}

bool OpenGLFramebuffer::Validate()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		break;

	case GL_FRAMEBUFFER_UNDEFINED:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_UNDEFINED!\n";
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT!\n";
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS!\n";
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT!\n";
		break;

	default:
		std::cout << "OpenGLFramebuffer::Validate(): Unknown error!\n";
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return (status == GL_FRAMEBUFFER_COMPLETE);
}

void OpenGLFramebuffer::Attach(GLenum target, GLuint tex, GLint level)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT)
		attach = &depthstencil;
	else
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];

	// must be the same or empty
	GL_ASSERT(attach->ID == tex || attach->ID == 0);

	attach->Type = GL_ATTACH_TYPE_TEXTURE;
	attach->ID = tex;

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, attach->ID, level);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Attach(GLenum target, GLuint tex, GLint face, GLint level)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT)
		attach = &depthstencil;
	else
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];

	// must be the same or empty
	GL_ASSERT(attach->ID == tex || attach->ID == 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, attach->ID, level);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Attach(GLenum target, GLuint renderbuffer)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT)
		attach = &depthstencil;
	else
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];

	// must be the same or empty
	GL_ASSERT(attach->ID == renderbuffer || attach->ID == 0);

	attach->Type = GL_ATTACH_TYPE_RENDERBUFFER;
	attach->ID = renderbuffer;

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glBindRenderbuffer(GL_RENDERBUFFER, attach->ID);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, GL_RENDERBUFFER, attach->ID);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Detach(GLenum target)
{
	Attachment* attach = nullptr;

	if (target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT)
		attach = &depthstencil;
	else
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];

	attach->ID = 0;

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::CopyDepthStencil(OpenGLFramebuffer* to, GLbitfield mask)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);

	if (to != nullptr)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->fboID);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	if (to != nullptr)
		glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, to->sizex, to->sizey, mask, GL_NEAREST);
	else
		glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, sizex, sizey, mask, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Resolve(OpenGLFramebuffer* to, GLbitfield mask)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);

	if (to != nullptr)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->fboID);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	if (mask & GL_COLOR_BUFFER_BIT) {
		for (int i = 0; i < 8; ++i) {
			if (rendertargets[i].ID != 0) {
				glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
				glDrawBuffer((to == 0) ? GL_BACK : GL_COLOR_ATTACHMENT0 + i);

				if (to != nullptr)
					glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, to->sizex, to->sizey, GL_COLOR_BUFFER_BIT, GL_LINEAR);
				else
					glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, sizex, sizey, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}
		}
	}

	if (mask & (GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT)) {
		if (to != nullptr)
			glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, to->sizex, to->sizey, mask & (~GL_COLOR_BUFFER_BIT), GL_NEAREST);
		else
			glBlitFramebuffer(0, 0, sizex, sizey, 0, 0, sizex, sizey, mask & (~GL_COLOR_BUFFER_BIT), GL_NEAREST);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (mask & GL_COLOR_BUFFER_BIT) {
		glReadBuffer(GL_BACK);
		glDrawBuffer(GL_BACK);
	}
}

void OpenGLFramebuffer::Set()
{
	GLenum buffs[8];
	GLsizei count = 0;

	for (int i = 0; i < 8; ++i) {
		if (rendertargets[i].ID != 0) {
			buffs[i] = GL_COLOR_ATTACHMENT0 + i;
			count = (i + 1);
		} else {
			buffs[i] = GL_NONE;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);

	if (count > 0)
		glDrawBuffers(count, buffs);

	glViewport(0, 0, sizex, sizey);
}

void OpenGLFramebuffer::Unset()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
}

// --- OpenGLScreenQuad impl --------------------------------------------------

OpenGLScreenQuad::OpenGLScreenQuad(const char* psfile, const char* defines)
{
	effect = nullptr;
	vertexlayout = 0;

	glGenVertexArrays(1, &vertexlayout);

	glBindVertexArray(vertexlayout);
	{
		// empty (will be generated in shader)
	}
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (psfile != nullptr) {
		GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, psfile, &effect, defines);
		effect->SetInt("sampler0", 0);
	}
}

OpenGLScreenQuad::~OpenGLScreenQuad()
{
	if (effect != nullptr)
		delete effect;

	if (vertexlayout != 0)
		glDeleteVertexArrays(1, &vertexlayout);
}

void OpenGLScreenQuad::Draw(bool useinternaleffect)
{
	if (useinternaleffect && effect != nullptr)
		effect->Begin();

	glBindVertexArray(vertexlayout);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	if (useinternaleffect && effect != nullptr)
		effect->End();
}

void OpenGLScreenQuad::SetTextureMatrix(const Math::Matrix& m)
{
	effect->SetMatrix("matTexture", m);
}

// --- Functions impl ---------------------------------------------------------

bool GLCreateCubeTextureFromDDS(const char* file, bool srgb, GLuint* out)
{
	DDS_Image_Info info;
	GLuint texid = OpenGLContentManager().IDTexture(file);

	if (texid != 0) {
		printf("Pointer %s\n", file);
		*out = texid;

		return true;
	}

	if (!LoadFromDDS(file, &info)) {
		std::cout << "Error: Could not load texture!\n";
		return false;
	}

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (info.MipLevels > 1)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLsizei pow2s = Math::NextPow2(info.Width);
	GLsizei facesize;
	GLenum format = info.Format;

	if (info.Format == GLFMT_DXT1 || info.Format == GLFMT_DXT5) {
		// compressed
		GLsizei size;
		GLsizei offset = 0;

		if (srgb) {
			if (format == GLFMT_DXT1)
				format = GLFMT_DXT1_sRGB;
			else
				format = GLFMT_DXT5_sRGB;
		}

		for (int i = 0; i < 6; ++i) {
			for (uint32_t j = 0; j < info.MipLevels; ++j) {
				size = Math::Max(1, pow2s >> j);
				facesize = GetCompressedLevelSize(info.Width, info.Height, j, info.Format);

				glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, map_Format_Internal[format],
					size, size, 0, facesize, (char*)info.Data + offset);

				offset += facesize;
			}
		}
	} else {
		// uncompressed
		GLsizei size;
		GLsizei offset = 0;
		GLsizei bytes = 4;

		if (info.Format == GLFMT_A16B16G16R16F)
			bytes = 8;

		for (int i = 0; i < 6; ++i) {
			for (uint32_t j = 0; j < info.MipLevels; ++j) {
				size = Math::Max(1, pow2s >> j);
				facesize = size * size * bytes;

				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, map_Format_Internal[info.Format], size, size, 0,
					map_Format_Format[info.Format], map_Format_Type[info.Format], (char*)info.Data + offset);

				offset += facesize;
			}
		}
	}

	if (info.Data)
		free(info.Data);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create texture!\n";
	} else {
		std::cout << "Created cube texture " << info.Width << "x" << info.Height << "\n";
	}

	*out = texid;
	OpenGLContentManager().RegisterTexture(file, texid);

	return (texid != 0);
}

bool GLCreateCubeTextureFromFiles(const char* files[6], bool srgb, GLuint* out)
{
	if (out == nullptr)
		return false;

	uint8_t* imgdata = nullptr;
	GLuint texid = OpenGLContentManager().IDTexture(files[0]);

	if (texid != 0) {
		printf("Pointer %s\n", files[0]);
		*out = texid;

		return true;
	}

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (int k = 0; k < 6; ++k) {
#ifdef _WIN32
		Gdiplus::Bitmap* bitmap = Win32LoadPicture(files[k]);

		if (bitmap == nullptr)
			return false;

		if (bitmap->GetLastStatus() != Gdiplus::Ok) {
			delete bitmap;
			return false;
		}

		Gdiplus::BitmapData data;

		bitmap->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
		{
			imgdata = new uint8_t[data.Width * data.Height * 4];
			memcpy(imgdata, data.Scan0, data.Width * data.Height * 4);

			for (UINT i = 0; i < data.Height; ++i) {
				// swap red and blue
				for (UINT j = 0; j < data.Width; ++j) {
					UINT index = (i * data.Width + j) * 4;
					Math::Swap<uint8_t>(imgdata[index + 0], imgdata[index + 2]);
				}

				// flip on X
				for (UINT j = 0; j < data.Width / 2; ++j) {
					UINT index1 = (i * data.Width + j) * 4;
					UINT index2 = (i * data.Width + (data.Width - j - 1)) * 4;

					Math::Swap<uint32_t>(*((uint32_t*)(imgdata + index1)), *((uint32_t*)(imgdata + index2)));
				}
			}
		}
		bitmap->UnlockBits(&data);

		delete bitmap;
		
		if (srgb)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + k, 0, GL_SRGB8_ALPHA8, data.Width, data.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);
		else
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + k, 0, GL_RGBA, data.Width, data.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);
#else
		// TODO:
#endif

		delete[] imgdata;
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create cube texture!";
	} else {
		std::cout << "Created cube texture\n";
	}

	*out = texid;
	OpenGLContentManager().RegisterTexture(files[0], texid);

	return (texid != 0);
}

bool GLCreateMesh(GLuint numvertices, GLuint numindices, GLuint options, OpenGLVertexElement* decl, OpenGLMesh** mesh)
{
	OpenGLMesh* glmesh = new OpenGLMesh();
	int numdeclelems = 0;

	for (int i = 0; i < 16; ++i) {
		++numdeclelems;

		if (decl[i].Stream == 0xff)
			break;
	}

	glGenBuffers(1, &glmesh->vertexbuffer);

	if (numvertices >= 0xffff)
		options |= GLMESH_32BIT;

	glmesh->meshoptions					= options;
	glmesh->numsubsets					= 1;
	glmesh->numvertices					= numvertices;
	glmesh->numindices					= numindices;
	glmesh->subsettable					= new OpenGLAttributeRange[1];

	glmesh->subsettable->AttribId		= 0;
	glmesh->subsettable->IndexCount		= numindices;
	glmesh->subsettable->IndexStart		= 0;
	glmesh->subsettable->PrimitiveType	= GLPT_TRIANGLELIST;
	glmesh->subsettable->VertexCount	= (numindices > 0 ? 0 : numvertices);
	glmesh->subsettable->VertexStart	= 0;
	glmesh->subsettable->Enabled		= GL_TRUE;

	glmesh->vertexdecl.Elements = new OpenGLVertexElement[numdeclelems];
	memcpy(glmesh->vertexdecl.Elements, decl, numdeclelems * sizeof(OpenGLVertexElement));

	// create vertex layout
	glmesh->RecreateVertexLayout();
	GL_ASSERT(glmesh->vertexdecl.Stride != 0);

	// allocate storage
	GLenum usage = ((options & GLMESH_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	GLuint istride = ((options & GLMESH_32BIT) ? 4 : 2);

	glBindBuffer(GL_ARRAY_BUFFER, glmesh->vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, numvertices * glmesh->vertexdecl.Stride, 0, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (numindices > 0) {
		glGenBuffers(1, &glmesh->indexbuffer);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glmesh->indexbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numindices * istride, 0, usage);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	(*mesh) = glmesh;
	return true;
}

bool GLCreateMeshFromQM(const char* file, OpenGLMesh** mesh, uint32_t options)
{
	static const uint8_t usages[] = {
		GLDECLUSAGE_POSITION,
		GLDECLUSAGE_POSITIONT,
		GLDECLUSAGE_COLOR,
		GLDECLUSAGE_BLENDWEIGHT,
		GLDECLUSAGE_BLENDINDICES,
		GLDECLUSAGE_NORMAL,
		GLDECLUSAGE_TEXCOORD,
		GLDECLUSAGE_TANGENT,
		GLDECLUSAGE_BINORMAL,
		GLDECLUSAGE_PSIZE,
		GLDECLUSAGE_TESSFACTOR
	};

	static const uint16_t elemsizes[6] = {
		1, 2, 3, 4, 4, 4
	};

	static const uint16_t elemstrides[6] = {
		4, 4, 4, 4, 1, 1
	};

	Math::AABox				box;
	OpenGLVertexElement*	decl;
	OpenGLAttributeRange*	table;
	OpenGLMaterial*			mat;
	Math::Color				color;

	std::string				basedir(file);
	std::string				str;
	FILE*					infile = nullptr;
	Math::Vector3			bbmin, bbmax;
	uint32_t				unused;
	uint32_t				version;
	uint32_t				numindices;
	uint32_t				numvertices;
	uint32_t				vstride;
	uint32_t				istride;
	uint32_t				numsubsets;
	uint32_t				numelems;
	uint16_t				tmp16;
	uint8_t					tmp8;
	void*					data = nullptr;
	char					buff[256];
	bool					success;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb");
#endif

	if (!infile)
		return false;

	basedir = basedir.substr(0, basedir.find_last_of('/') + 1);

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	table = new OpenGLAttributeRange[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new OpenGLVertexElement[numelems + 1];

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
		decl[i].Offset = vstride;

		vstride += elemsizes[decl[i].Type] * elemstrides[decl[i].Type];
	}

	decl[numelems].Stream = 0xff;
	decl[numelems].Offset = 0;
	decl[numelems].Type = 0;
	decl[numelems].Usage = 0;
	decl[numelems].UsageIndex = 0;

	// create mesh
	success = GLCreateMesh(numvertices, numindices, options | (istride == 4 ? GLMESH_32BIT : 0), decl, mesh);

	if (!success)
		goto _fail;

	(*mesh)->LockVertexBuffer(0, 0, GLLOCK_DISCARD, &data);
	fread(data, vstride, numvertices, infile);
	(*mesh)->UnlockVertexBuffer();

	if ((options & GLMESH_32BIT) && istride == 2) {
		uint16_t* tmpdata = new uint16_t[numindices];
		fread(tmpdata, istride, numindices, infile);

		(*mesh)->LockIndexBuffer(0, 0, GLLOCK_DISCARD, &data);
		{
			for (uint16_t i = 0; i < numindices; ++i)
				*((uint32_t*)data + i) = tmpdata[i];
		}
		(*mesh)->UnlockIndexBuffer();

		delete[] tmpdata;
	} else {
		(*mesh)->LockIndexBuffer(0, 0, GLLOCK_DISCARD, &data);
		fread(data, istride, numindices, infile);
		(*mesh)->UnlockIndexBuffer();
	}

	if (version >= 1) {
		fread(&unused, 4, 1, infile);

		if (unused > 0)
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	// attribute table
	(*mesh)->materials = new OpenGLMaterial[numsubsets];

	for (uint32_t i = 0; i < numsubsets; ++i) {
		OpenGLAttributeRange& subset = table[i];
		mat = ((*mesh)->materials + i);

		subset.AttribId = i;
		subset.PrimitiveType = GLPT_TRIANGLELIST;
		subset.Enabled = GL_TRUE;

		fread(&subset.IndexStart, 4, 1, infile);
		fread(&subset.VertexStart, 4, 1, infile);
		fread(&subset.VertexCount, 4, 1, infile);
		fread(&subset.IndexCount, 4, 1, infile);

		fread(bbmin, sizeof(float), 3, infile);
		fread(bbmax, sizeof(float), 3, infile);

		box.Add(bbmin);
		box.Add(bbmax);

		(*mesh)->boundingbox.Add(bbmin);
		(*mesh)->boundingbox.Add(bbmax);

		ReadString(infile, buff);
		ReadString(infile, buff);

		bool hasmaterial = (buff[1] != ',');

		if (hasmaterial) {
			fread(&color, sizeof(Math::Color), 1, infile);
			mat->Ambient = color;

			fread(&color, sizeof(Math::Color), 1, infile);
			mat->Diffuse = color;

			fread(&color, sizeof(Math::Color), 1, infile);
			mat->Specular = color;

			fread(&color, sizeof(Math::Color), 1, infile);
			mat->Emissive = color;

			if (version >= 2)
				fseek(infile, 16, SEEK_CUR);	// uvscale

			fread(&mat->Power, sizeof(float), 1, infile);
			fread(&mat->Diffuse.a, sizeof(float), 1, infile);

			fread(&unused, 4, 1, infile);
			ReadString(infile, buff);

			if (buff[1] != ',') {
				str = basedir + buff;
				GLCreateTextureFromFile(str.c_str(), true, &mat->Texture);
			}

			ReadString(infile, buff);

			if (buff[1] != ',') {
				str = basedir + buff;
				GLCreateTextureFromFile(str.c_str(), false, &mat->NormalMap);
			}

			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
		} else {
			color = Math::Color(1, 1, 1, 1);

			memcpy(&mat->Diffuse, &color, sizeof(Math::Color));
			memcpy(&mat->Specular, &color, sizeof(Math::Color));

			color = Math::Color(0, 0, 0, 1);

			memcpy(&mat->Emissive, &color, sizeof(Math::Color));
			memcpy(&mat->Ambient, &color, sizeof(Math::Color));

			mat->Power = 80.0f;
		}

		// texture info
		ReadString(infile, buff);

		if (buff[1] != ',' && mat->Texture == 0) {
			str = basedir + buff;
			GLCreateTextureFromFile(str.c_str(), true, &mat->Texture);
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
	(*mesh)->SetAttributeTable(table, numsubsets);

	// printf some info
	Math::GetFile(str, file);
	box.GetSize(bbmin);

	printf("Loaded mesh '%s': size = (%.3f, %.3f, %.3f)\n", str.c_str(), bbmin[0], bbmin[1], bbmin[2]);

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return success;
}

bool GLCreateNormalizationCubemap(GLuint* out)
{
	if (out == nullptr)
		return false;

	float		tmp[4];
	uint8_t*	bytes = new uint8_t[128 * 128 * 4];
	GLuint		ret = 0;
	int			index;
	
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ret);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = 64;
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = 64 - (j + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = 64;
			tmp[2] = -64 + (i + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = 64;
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = -64;
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = -64 + (j + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = -64;
			tmp[2] = 64 - (i + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			tmp[0] = 64 - (j + 0.5f);
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = -64;
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (uint8_t)(tmp[2] * 255.0f);
			bytes[index + 1] = (uint8_t)(tmp[1] * 255.0f);
			bytes[index + 0] = (uint8_t)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	delete[] bytes;

	*out = ret;
	return true;
}

bool GLCreateTexture(GLsizei width, GLsizei height, GLint miplevels, OpenGLFormat format, GLuint* out, void* data)
{
	GLuint texid = 0;

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (miplevels != 1)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(
		GL_TEXTURE_2D, 0, map_Format_Internal[format], width, height, 0,
		map_Format_Format[format], map_Format_Type[format], data);

	if (miplevels != 1)
		glGenerateMipmap(GL_TEXTURE_2D);

	*out = texid;
	return true;
}

bool GLCreateTextureFromDDS(const char* file, bool srgb, GLuint* out)
{
	DDS_Image_Info info;
	GLuint texid = OpenGLContentManager().IDTexture(file);

	if (texid != 0) {
		printf("Pointer %s\n", file);
		*out = texid;

		return true;
	}

	if (!LoadFromDDS(file, &info)) {
		std::cout << "Error: Could not load texture!\n";
		return false;
	}

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (info.MipLevels > 1)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLsizei pow2w = Math::NextPow2(info.Width);
	GLsizei pow2h = Math::NextPow2(info.Height);
	GLsizei mipsize;
	GLuint format = info.Format;

	if (info.Format == GLFMT_DXT1 || info.Format == GLFMT_DXT5) {
		if (srgb) {
			if (info.Format == GLFMT_DXT1)
				format = GLFMT_DXT1_sRGB;
			else
				format = GLFMT_DXT5_sRGB;
		}

		// compressed
		GLsizei width = info.Width;
		GLsizei height = info.Height;
		GLsizei offset = 0;

		for (uint32_t j = 0; j < info.MipLevels; ++j) {
			mipsize = GetCompressedLevelSize(info.Width, info.Height, j, info.Format);

			glCompressedTexImage2D(GL_TEXTURE_2D, j, map_Format_Internal[format],
				width, height, 0, mipsize, (char*)info.Data + offset);

			offset += mipsize;

			width = (pow2w >> (j + 1));
			height = (pow2h >> (j + 1));
		}
	} else {
		// uncompressed
		uint32_t bytes = 4;

		if (info.Format == GLFMT_G32R32F) {
			bytes = 8;
		} else if (info.Format == GLFMT_G16R16F) {
			bytes = 4;
		} else if (info.Format == GLFMT_R8G8B8 || info.Format == GLFMT_B8G8R8) {
			if (srgb) {
				// see the enum
				format = info.Format + 1;
			}

			bytes = 3;
		}

		mipsize = info.Width * info.Height * bytes;

		// TODO: mipmap
		glTexImage2D(GL_TEXTURE_2D, 0, map_Format_Internal[format], info.Width, info.Height, 0,
			map_Format_Format[format], map_Format_Type[format], (char*)info.Data);

		if (info.MipLevels > 1)
			glGenerateMipmap(GL_TEXTURE_2D);
	}

	if (info.Data)
		free(info.Data);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create texture!\n";
	} else {
		std::cout << "Created texture " << info.Width << "x" << info.Height << "\n";
	}

	*out = texid;
	OpenGLContentManager().RegisterTexture(file, texid);

	return (texid != 0);
}

bool GLCreateTextureFromFile(const char* file, bool srgb, GLuint* out, GLuint flags)
{
	if (out == nullptr)
		return false;

	std::string	ext;
	uint8_t*	imgdata	= nullptr;
	GLuint		texid	= OpenGLContentManager().IDTexture(file);
	GLsizei		width	= 0;
	GLsizei		height	= 0;

	if (texid != 0) {
		printf("Pointer %s\n", file);
		*out = texid;

		return true;
	}

	Math::GetExtension(ext, file);

	if (ext == "dds")
		return GLCreateTextureFromDDS(file, srgb, out);

#ifdef _WIN32
	Gdiplus::Bitmap* bitmap = Win32LoadPicture(file);

	if (bitmap == nullptr)
		return false;

	if (bitmap->GetLastStatus() != Gdiplus::Ok) {
		delete bitmap;
		return false;
	}

	Gdiplus::BitmapData data;

	bitmap->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	{
		imgdata = new uint8_t[data.Width * data.Height * 4];
		memcpy(imgdata, data.Scan0, data.Width * data.Height * 4);

		for (UINT i = 0; i < data.Height; ++i) {
			// swap red and blue
			for (UINT j = 0; j < data.Width; ++j) {
				UINT index = (i * data.Width + j) * 4;
				Math::Swap<uint8_t>(imgdata[index + 0], imgdata[index + 2]);
			}

			// flip on X
			if (flags & GLTEX_FLIPX) {
				for (UINT j = 0; j < data.Width / 2; ++j) {
					UINT index1 = (i * data.Width + j) * 4;
					UINT index2 = (i * data.Width + (data.Width - j - 1)) * 4;

					Math::Swap<uint32_t>(*((uint32_t*)(imgdata + index1)), *((uint32_t*)(imgdata + index2)));
				}
			}
		}

		if (flags & GLTEX_FLIPY) {
			// flip on Y
			for (UINT j = 0; j < data.Height / 2; ++j) {
				for (UINT i = 0; i < data.Width; ++i) {
					//UINT index1 = (i * data.Width + j) * 4;
					//UINT index2 = (i * data.Width + (data.Width - j - 1)) * 4;

					UINT index1 = (j * data.Width + i) * 4;
					UINT index2 = ((data.Height - j - 1) * data.Width + i) * 4;

					Math::Swap<uint32_t>(*((uint32_t*)(imgdata + index1)), *((uint32_t*)(imgdata + index2)));
				}
			}
		}
	}
	bitmap->UnlockBits(&data);

	width = data.Width;
	height = data.Height;

	delete bitmap;
#else
	// TODO:
#endif

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (srgb)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);

	glGenerateMipmap(GL_TEXTURE_2D);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create texture!\n";
	} else {
		std::cout << "Created texture " << width << "x" << height << "\n";
	}

	delete[] imgdata;

	*out = texid;
	OpenGLContentManager().RegisterTexture(file, texid);

	return (texid != 0);
}

bool GLCreateTextureArrayFromFiles(const std::string* files, uint32_t numfiles, bool srgb, GLuint* out)
{
	if (out == nullptr)
		return false;

	uint8_t*	imgdata	= nullptr;
	GLuint		texid	= 0;
	GLsizei		width	= 0;
	GLsizei		height	= 0;

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texid);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef _WIN32
	std::vector<Gdiplus::Bitmap*> bitmaps;

	bitmaps.resize(numfiles, nullptr);

	for (uint32_t i = 0; i < numfiles; ++i) {
		bitmaps[i] = Win32LoadPicture(files[i]);

		if (bitmaps[i] == nullptr) {
			width = height = 0;
			break;
		}

		if (bitmaps[i]->GetLastStatus() != Gdiplus::Ok) {
			width = height = 0;
			break;
		}

		width = Math::Max<GLsizei>(width, bitmaps[i]->GetWidth());
		height = Math::Max<GLsizei>(height, bitmaps[i]->GetHeight());
	}

	if (width == 0 || height == 0) {
		for (Gdiplus::Bitmap* bitmap : bitmaps)
			delete bitmap;

		return false;
	}

	// resize & upload
	Gdiplus::Bitmap* scaled = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	Gdiplus::Graphics graphics(scaled);
	Gdiplus::Rect target(0, 0, width, height);
	Gdiplus::BitmapData data;
	GLsizei miplevels = Math::Max<uint32_t>(1, (uint32_t)floor(log(Math::Max<double>(width, height)) / 0.69314718055994530941723212));

	graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);

	// to avoid ringing
	Gdiplus::ImageAttributes wrapmode;
	wrapmode.SetWrapMode(Gdiplus::WrapModeTileFlipXY);

	if (srgb)
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, miplevels, GL_SRGB8_ALPHA8, width, height, numfiles);
	else
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, miplevels, GL_RGBA8, width, height, numfiles);

	imgdata = new uint8_t[width * height * 4];

	for (size_t i = 0; i < bitmaps.size(); ++i) {
		Gdiplus::Bitmap* bitmap = bitmaps[i];

		graphics.DrawImage(bitmap, target, 0, 0, bitmap->GetWidth(), bitmap->GetHeight(), Gdiplus::UnitPixel, &wrapmode);

		scaled->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
		{
			memcpy(imgdata, data.Scan0, data.Width * data.Height * 4);

			GL_ASSERT(data.Width == width);
			GL_ASSERT(data.Height == height);

			for (UINT j = 0; j < data.Height; ++j) {
				// swap red and blue
				for (UINT k = 0; k < data.Width; ++k) {
					UINT index = (j * data.Width + k) * 4;
					Math::Swap<uint8_t>(imgdata[index + 0], imgdata[index + 2]);
				}
			}
		}
		scaled->UnlockBits(&data);

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);
	}
	
	delete[] imgdata;
	delete scaled;

	for (Gdiplus::Bitmap* bitmap : bitmaps)
		delete bitmap;
#else
	// TODO:
#endif

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create texture array!\n";
	} else {
		std::cout << "Created texture array " << width << "x" << height << "\n";
	}

	*out = texid;

	return (texid != 0);
}

bool GLCreateVolumeTextureFromFile(const char* file, bool srgb, GLuint* out)
{
	DDS_Image_Info info;
	GLuint texid = OpenGLContentManager().IDTexture(file);

	if (texid != 0) {
		printf("Pointer %s\n", file);
		*out = texid;

		return true;
	}

	if (!LoadFromDDS(file, &info)) {
		std::cout << "Error: Could not load texture!\n";
		return false;
	}

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_3D, texid);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (info.MipLevels > 1)
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLenum format = info.Format;

	if (info.Format == GLFMT_DXT1 || info.Format == GLFMT_DXT5) {
		// compressed
		GLsizei size;
		GLsizei offset = 0;

		if (srgb) {
			if (format == GLFMT_DXT1)
				format = GLFMT_DXT1_sRGB;
			else
				format = GLFMT_DXT5_sRGB;
		}

		for (uint32_t j = 0; j < info.MipLevels; ++j) {
			size = GetCompressedLevelSize(info.Width, info.Height, info.Depth, j, info.Format);

			glCompressedTexImage3D(GL_TEXTURE_3D, j, map_Format_Internal[format],
				info.Width, info.Height, info.Depth, 0, size, (char*)info.Data + offset);

			offset += size * info.Depth;
		}
	} else {
		// TODO:
	}

	if (info.Data)
		free(info.Data);

	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		glDeleteTextures(1, &texid);
		texid = 0;

		std::cout << "Error: Could not create texture!\n";
	} else {
		std::cout << "Created volume texture " << info.Width << "x" << info.Height << "x" << info.Depth << "\n";
	}

	*out = texid;
	OpenGLContentManager().RegisterTexture(file, texid);

	return (texid != 0);
}

OpenGLMesh* GLCreateDebugBox()
{
	OpenGLMesh* mesh = nullptr;

	OpenGLVertexElement decl[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	if (!GLCreateMesh(8, 24, 0, decl, &mesh))
		return nullptr;

	Math::Vector3* vdata = nullptr;
	GLushort* idata = nullptr;
	
	mesh->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	mesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);

	vdata[0] = Math::Vector3(-0.5f, -0.5f, -0.5f);
	vdata[1] = Math::Vector3(-0.5f, -0.5f, 0.5f);
	vdata[2] = Math::Vector3(-0.5f, 0.5f, -0.5f);
	vdata[3] = Math::Vector3(-0.5f, 0.5f, 0.5f);
	vdata[4] = Math::Vector3(0.5f, -0.5f, -0.5f);
	vdata[5] = Math::Vector3(0.5f, -0.5f, 0.5f);
	vdata[6] = Math::Vector3(0.5f, 0.5f, -0.5f);
	vdata[7] = Math::Vector3(0.5f, 0.5f, 0.5f);

	idata[0] = 0;	idata[8] = 4;	idata[16] = 0;
	idata[1] = 1;	idata[9] = 5;	idata[17] = 4;
	idata[2] = 0;	idata[10] = 4;	idata[18] = 2;
	idata[3] = 2;	idata[11] = 6;	idata[19] = 6;
	idata[4] = 1;	idata[12] = 5;	idata[20] = 1;
	idata[5] = 3;	idata[13] = 7;	idata[21] = 5;
	idata[6] = 2;	idata[14] = 6;	idata[22] = 3;
	idata[7] = 3;	idata[15] = 7;	idata[23] = 7;

	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	mesh->GetAttributeTable()[0].PrimitiveType = GLPT_LINELIST;
	return mesh;
}

GLuint GLCompileShaderFromFile(GLenum type, const char* file, const char* defines)
{
	std::string	source;
	char		log[1024];
	size_t		pos;
	FILE*		infile = nullptr;
	GLuint		shader = 0;
	GLint		length;
	GLint		success;
	GLint		deflength = 0;

	if (file == nullptr)
		return 0;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#elif defined(__APPLE__)
	std::string resfile = GetResource(file);
	infile = fopen(resfile.c_str(), "rb");
#else
	infile = fopen(file, "rb");
#endif

	if (infile == nullptr)
		return 0;

	fseek(infile, 0, SEEK_END);
	length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	if (defines != nullptr)
		deflength = (GLint)strlen(defines);

	source.reserve(length + deflength);
	source.resize(length);

	fread(&source[0], 1, length, infile);

	// add defines
	if (defines != nullptr) {
		pos = source.find("#version");
		pos = source.find('\n', pos) + 1;

		source.insert(pos, defines);
	}

	fclose(infile);

	// process includes (non-recursive)
	pos = source.find("#include");

	while (pos != std::string::npos) {
		size_t start = source.find('\"', pos) + 1;
		size_t end = source.find('\"', start);

		std::string incfile(source.substr(start, end - start));
		std::string path;
		std::string incsource;

		Math::GetPath(path, file);

#ifdef _MSC_VER
		fopen_s(&infile, (path + incfile).c_str(), "rb");
#else
		infile = fopen((path + incfile).c_str(), "rb");
#endif

		if (infile) {
			fseek(infile, 0, SEEK_END);
			length = ftell(infile);
			fseek(infile, 0, SEEK_SET);

			incsource.resize(length);

			fread(&incsource[0], 1, length, infile);
			fclose(infile);

			source.replace(pos, end - pos + 1, incsource);
		}

		pos = source.find("#include", end);
	}

	shader = glCreateShader(type);
	length = (GLint)source.length();

	const GLcharARB* sourcedata = (const GLcharARB*)source.data();

	glShaderSource(shader, 1, &sourcedata, &length);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success != GL_TRUE) {
		glGetShaderInfoLog(shader, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";
		glDeleteShader(shader);

		return 0;
	}

	return shader;
}

bool GLCreateEffectFromFile(const char* vsfile, const char* tcsfile, const char* tesfile, const char* gsfile, const char* psfile, OpenGLEffect** effect, const char* defines)
{
	char			log[1024];
	OpenGLEffect*	neweffect;
	GLuint			vertexshader		= 0;
	GLuint			tesscontrolshader	= 0;
	GLuint			tessshader			= 0;
	GLuint			tessevalshader		= 0;
	GLuint			geometryshader		= 0;
	GLuint			fragmentshader		= 0;
	GLint			success				= GL_FALSE;
	GLint			length				= 0;

	// these are mandatory
	if (0 == (vertexshader = GLCompileShaderFromFile(GL_VERTEX_SHADER, vsfile, defines)))
		return false;

	if (0 == (fragmentshader = GLCompileShaderFromFile(GL_FRAGMENT_SHADER, psfile, defines))) {
		glDeleteShader(vertexshader);
		return false;
	}

	// others are optional
	if (tesfile != nullptr) {
		if (0 == (tessevalshader = GLCompileShaderFromFile(GL_TESS_EVALUATION_SHADER, tesfile, defines))) {
			glDeleteShader(vertexshader);
			glDeleteShader(fragmentshader);

			return false;
		}
	}

	
	if (tcsfile != nullptr) {
		if (0 == (tesscontrolshader = GLCompileShaderFromFile(GL_TESS_CONTROL_SHADER, tcsfile, defines))) {
			glDeleteShader(vertexshader);
			glDeleteShader(fragmentshader);

			if (tessevalshader != 0)
				glDeleteShader(tessevalshader);

			return false;
		}
	}

	if (gsfile != nullptr) {
		if (0 == (geometryshader = GLCompileShaderFromFile(GL_GEOMETRY_SHADER, gsfile, defines))) {
			glDeleteShader(vertexshader);
			glDeleteShader(fragmentshader);

			if (tessevalshader != 0)
				glDeleteShader(tessevalshader);

			if (tesscontrolshader != 0)
				glDeleteShader(tesscontrolshader);

			return false;
		}
	}

	neweffect = new OpenGLEffect();
	neweffect->program = glCreateProgram();

	glAttachShader(neweffect->program, vertexshader);
	glAttachShader(neweffect->program, fragmentshader);

	if (tessevalshader != 0)
		glAttachShader(neweffect->program, tessevalshader);

	if (tesscontrolshader != 0)
		glAttachShader(neweffect->program, tesscontrolshader);

	if (geometryshader != 0)
		glAttachShader(neweffect->program, geometryshader);

	glLinkProgram(neweffect->program);
	glGetProgramiv(neweffect->program, GL_LINK_STATUS, &success);

	if (success != GL_TRUE) {
		glGetProgramInfoLog(neweffect->program, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		glDeleteProgram(neweffect->program);
		delete neweffect;

		return false;
	}

	neweffect->BindAttributes();
	neweffect->QueryUniforms();

	// delete shaders
	glDetachShader(neweffect->program, vertexshader);
	glDeleteShader(vertexshader);

	glDetachShader(neweffect->program, fragmentshader);
	glDeleteShader(fragmentshader);

	if (tessevalshader != 0) {
		glDetachShader(neweffect->program, tessevalshader);
		glDeleteShader(tessevalshader);
	}

	if (tesscontrolshader != 0) {
		glDetachShader(neweffect->program, tesscontrolshader);
		glDeleteShader(tesscontrolshader);
	}

	if (geometryshader != 0) {
		glDetachShader(neweffect->program, geometryshader);
		glDeleteShader(geometryshader);
	}

	(*effect) = neweffect;
	return true;
}

bool GLCreateComputeProgramFromFile(const char* csfile, OpenGLEffect** effect, const char* defines)
{
	char			log[1024];
	OpenGLEffect*	neweffect;
	GLuint			shader = 0;
	GLint			success;
	GLint			length;

	if (0 == (shader = GLCompileShaderFromFile(GL_COMPUTE_SHADER, csfile, defines)))
		return false;

	neweffect = new OpenGLEffect();
	neweffect->program = glCreateProgram();

	glAttachShader(neweffect->program, shader);
	glLinkProgram(neweffect->program);
	glGetProgramiv(neweffect->program, GL_LINK_STATUS, &success);

	if (success != GL_TRUE) {
		glGetProgramInfoLog(neweffect->program, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		glDeleteProgram(neweffect->program);
		delete neweffect;

		return false;
	}

	neweffect->QueryUniforms();

	glDetachShader(neweffect->program, shader);
	glDeleteShader(shader);

	(*effect) = neweffect;
	return true;
}

void GLRenderText(const std::string& str, GLuint tex, GLsizei width, GLsizei height)
{
#ifdef _WIN32
	GLRenderTextEx(str, tex, width, height, L"Arial", 1, Gdiplus::FontStyleBold, 25);
#elif defined(__APPLE__)
	GLRenderTextEx(str, tex, width, height, L"Arial", true, kCTFontTraitBold, 25.0f);
#endif
}

void GLRenderTextEx(const std::string& str, GLuint tex, GLsizei width, GLsizei height, const WCHAR* face, bool border, int style, float emsize)
{
	if (tex == 0)
		return;

#ifdef _WIN32
	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(face);
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(border ? fill : outline);
	std::wstring				wstr(str.begin(), str.end());

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

	memset(&data, 0, sizeof(Gdiplus::BitmapData));
	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data.Scan0);

	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
#elif defined(__APPLE__)
	CGColorSpaceRef	colorspace		= CGColorSpaceCreateDeviceRGB();
	CGContextRef 	cgcontext 		= CGBitmapContextCreate(NULL, width, height, 8, width * 4, colorspace, kCGImageAlphaNoneSkipLast);
	
	CFStringEncoding encoding = (CFByteOrderLittleEndian == CFByteOrderGetCurrent()) ? kCFStringEncodingUTF32LE : kCFStringEncodingUTF32BE;
	int facelen = (int)wcslen(face);

	CFStringRef		font_name		= CFStringCreateWithBytes(NULL, (const UInt8*)face, (facelen * sizeof(wchar_t)), encoding, false);
	//CFStringRef 	font_name 		= CFStringCreateWithCString(NULL, face, kCFStringEncodingMacRoman);
	
	CTFontRef 		basefont		= CTFontCreateWithName(font_name, emsize, NULL);
	CTFontRef		font			= CTFontCreateCopyWithSymbolicTraits(basefont, emsize, NULL, style, style);
	CGColorRef 		strokecolor		= CGColorCreateGenericRGB(0, 0, 0, 1);
	CGColorRef 		fillcolor 		= CGColorCreateGenericRGB(1, 1, 1, 1);
	
	CFStringRef 	keys[] 			= { kCTFontAttributeName, kCTForegroundColorAttributeName };
	CFTypeRef 		values[] 		= { font, fillcolor };
	CFDictionaryRef	attributes		= CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys, (const void**)&values, sizeof(keys) / sizeof(keys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	
	CFRelease(font_name);
	CFRelease(font);
	CFRelease(basefont);
	CGColorRelease(strokecolor);
	CGColorRelease(fillcolor);
	
	CGRect					rect		= CGRectMake(0, 0, width, height);
	CGMutablePathRef		path		= CGPathCreateMutable();
	
	CGPathAddRect(path, NULL, rect);
	
	// draw text to CG context
	CFStringRef				cfstr		= CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)str.c_str (), str.length() * sizeof(char), kCFStringEncodingUTF8, false);
	CFAttributedStringRef	attrstr		= CFAttributedStringCreate(kCFAllocatorDefault, cfstr, attributes);
	CTFramesetterRef		framesetter	= CTFramesetterCreateWithAttributedString((CFAttributedStringRef)attrstr);
	CTFrameRef				frame		= CTFramesetterCreateFrame(framesetter, CFRangeMake(0, CFAttributedStringGetLength(attrstr)), path, NULL);

	//CGContextSetStrokeColorWithColor(cgcontext, stroke);
	//CGContextSetFillColorWithColor(cgcontext, stroke);
	//CGContextSetTextDrawingMode(cgcontext, kCGTextFillStroke);
	
	CGContextSetTextPosition(cgcontext, 0.0f, height - emsize);
	CTFrameDraw(frame, cgcontext);
	
	// copy to texture
	void* data = CGBitmapContextGetData(cgcontext);
	
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	// clean up
	CFRelease(frame);
	CFRelease(path);
	CFRelease(framesetter);
	CFRelease(cfstr);

	CGColorSpaceRelease(colorspace);
	CGContextRelease(cgcontext);
#endif
}

void GLSetTexture(GLenum unit, GLenum target, GLuint texture)
{
	glActiveTexture(unit);
	glBindTexture(target, texture);
}
