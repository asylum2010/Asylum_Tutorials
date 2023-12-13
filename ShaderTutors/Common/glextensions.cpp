
// --- Includes ---------------------------------------------------------------

#include "glextensions.h"
#include <cassert>

#ifdef _WIN32
#	pragma warning (disable:4996)

#	include <Windows.h>
#endif

// --- Defines ----------------------------------------------------------------

#define GET_ADDRESS(var, type, name) \
	if (coreprofile) { \
		var = (type)wglGetProcAddress(#var); \
		if (var == 0) \
			var = (type)wglGetProcAddress(name); \
	} else { \
		var = (type)wglGetProcAddress(name); \
	} \
	assert(var != 0);
// END

// --- OpenGL functions -------------------------------------------------------

#ifdef _WIN32
PFNGLACTIVETEXTUREARBPROC						glActiveTexture = 0;
PFNGLCLIENTACTIVETEXTUREARBPROC					glClientActiveTexture = 0;
PFNGLDELETEBUFFERSARBPROC						glDeleteBuffers = 0;
PFNGLBINDBUFFERARBPROC							glBindBuffer = 0;
PFNGLGENBUFFERSARBPROC							glGenBuffers = 0;
PFNGLBUFFERDATAARBPROC							glBufferData = 0;
PFNGLBUFFERSUBDATAARBPROC						glBufferSubData = 0;
PFNGLTEXIMAGE3DPROC								glTexImage3D = 0;
PFNGLTEXIMAGE2DMULTISAMPLEPROC					glTexImage2DMultisample = 0;
PFNGLTEXSUBIMAGE3DPROC							glTexSubImage3D = 0;

PFNGLCREATEPROGRAMOBJECTARBPROC					glCreateProgram = 0;
PFNGLCREATESHADEROBJECTARBPROC					glCreateShader = 0;
PFNGLSHADERSOURCEARBPROC						glShaderSource = 0;
PFNGLCOMPILESHADERARBPROC						glCompileShader = 0;
PFNGLATTACHOBJECTARBPROC						glAttachShader = 0;
PFNGLDETACHOBJECTARBPROC						glDetachShader = 0;
PFNGLLINKPROGRAMARBPROC							glLinkProgram = 0;
PFNGLGETINFOLOGARBPROC							glGetShaderInfoLog = 0;
PFNGLGETINFOLOGARBPROC							glGetProgramInfoLog = 0;
PFNGLGETUNIFORMLOCATIONARBPROC					glGetUniformLocation = 0;

PFNGLUNIFORMMATRIX2FVARBPROC					glUniformMatrix2fv = 0;
PFNGLUNIFORMMATRIX3FVARBPROC					glUniformMatrix3fv = 0;
PFNGLUNIFORMMATRIX4FVARBPROC					glUniformMatrix4fv = 0;

PFNGLUNIFORM1IARBPROC							glUniform1i = 0;
PFNGLUNIFORM4UIPROC								glUniform4ui = 0;
PFNGLUNIFORM4UIVPROC							glUniform4uiv = 0;
PFNGLUNIFORM1FARBPROC							glUniform1f = 0;
PFNGLUNIFORM2FARBPROC							glUniform2f = 0;
PFNGLUNIFORM3FARBPROC							glUniform3f = 0;
PFNGLUNIFORM4FARBPROC							glUniform4f = 0;
PFNGLUNIFORM1FVARBPROC							glUniform1fv = 0;
PFNGLUNIFORM2FVARBPROC							glUniform2fv = 0;
PFNGLUNIFORM3FVARBPROC							glUniform3fv = 0;
PFNGLUNIFORM4FVARBPROC							glUniform4fv = 0;

PFNGLGETPROGRAMIVPROC							glGetProgramiv = 0;
PFNGLGETSHADERIVPROC							glGetShaderiv = 0;
PFNGLDELETEPROGRAMPROC							glDeleteProgram = 0;
PFNGLDELETESHADERPROC							glDeleteShader = 0;

PFNGLUSEPROGRAMOBJECTARBPROC					glUseProgram = 0;
PFNGLGETACTIVEUNIFORMARBPROC					glGetActiveUniform = 0;
PFNGLGETACTIVEATTRIBPROC						glGetActiveAttrib = 0;
PFNGLGETATTRIBLOCATIONPROC						glGetAttribLocation = 0;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC				glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC			glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERARBPROC					glVertexAttribPointer = 0;
PFNGLBINDATTRIBLOCATIONARBPROC					glBindAttribLocation = 0;
PFNGLBINDFRAGDATALOCATIONPROC					glBindFragDataLocation = 0;

PFNGLGENFRAMEBUFFERSEXTPROC						glGenFramebuffers = 0;
PFNGLGENRENDERBUFFERSEXTPROC					glGenRenderbuffers = 0;
PFNGLBINDFRAMEBUFFEREXTPROC						glBindFramebuffer = 0;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC				glFramebufferTexture2D = 0;
PFNGLBINDRENDERBUFFEREXTPROC					glBindRenderbuffer = 0;
PFNGLRENDERBUFFERSTORAGEEXTPROC					glRenderbufferStorage = 0;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC				glFramebufferRenderbuffer = 0;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC				glCheckFramebufferStatus = 0;
PFNGLDELETEFRAMEBUFFERSEXTPROC					glDeleteFramebuffers = 0;
PFNGLDELETERENDERBUFFERSEXTPROC					glDeleteRenderbuffers = 0;
PFNGLGENERATEMIPMAPEXTPROC						glGenerateMipmap = 0;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC		glRenderbufferStorageMultisample = 0;
PFNGLBLITFRAMEBUFFEREXTPROC						glBlitFramebuffer = 0;
PFNGLFRAMEBUFFERTEXTURELAYERPROC				glFramebufferTextureLayer = 0;

PFNGLGETBUFFERSUBDATAARBPROC					glGetBufferSubData = 0;
PFNGLMAPBUFFERARBPROC							glMapBuffer = 0;
PFNGLMAPBUFFERRANGEPROC							glMapBufferRange = 0;
PFNGLUNMAPBUFFERARBPROC							glUnmapBuffer = 0;
PFNGLCOMPRESSEDTEXIMAGE3DPROC					glCompressedTexImage3D = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC					glCompressedTexImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE1DPROC					glCompressedTexImage1D = 0;
PFNGLDRAWBUFFERSARBPROC							glDrawBuffers = 0;
PFNGLDRAWRANGEELEMENTSPROC						glDrawRangeElements = 0;
PFNGLMULTIDRAWELEMENTSPROC						glMultiDrawElements = 0;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC					glFlushMappedBufferRange = 0;
PFNGLGETVERTEXATTRIBIVPROC						glGetVertexAttribiv = 0;
PFNGLGETVERTEXATTRIBFVPROC						glGetVertexAttribfv = 0;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC				glGetActiveUniformBlockiv = 0;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC				glGetActiveUniformBlockName = 0;

PFNGLBLENDEQUATIONPROC							glBlendEquation = 0;
PFNGLBLENDEQUATIONSEPARATEPROC					glBlendEquationSeparate = 0;
PFNGLBLENDFUNCSEPARATEPROC						glBlendFuncSeparate = 0;
PFNGLSTENCILFUNCSEPARATEPROC					glStencilFuncSeparate = 0;
PFNGLSTENCILMASKSEPARATEPROC					glStencilMaskSeparate = 0;
PFNGLSTENCILOPSEPARATEPROC						glStencilOpSeparate = 0;
PFNGLCOLORMASKIPROC								glColorMaski = 0;

PFNGLGENQUERIESPROC								glGenQueries = 0;
PFNGLDELETEQUERIESPROC							glDeleteQueries = 0;
PFNGLBEGINQUERYPROC								glBeginQuery = 0;
PFNGLENDQUERYPROC								glEndQuery = 0;
PFNGLGETQUERYOBJECTUIVPROC						glGetQueryObjectuiv = 0;
PFNGLGETBUFFERPARAMETERIVPROC					glGetBufferParameteriv = 0;

// 3.1
PFNGLDRAWARRAYSINSTANCEDPROC					glDrawArraysInstanced = 0;
PFNGLDRAWELEMENTSINSTANCEDPROC					glDrawElementsInstanced = 0;
PFNGLTEXBUFFERPROC								glTexBuffer = 0;
PFNGLPRIMITIVERESTARTINDEXPROC					glPrimitiveRestartIndex = 0;

// 3.2
PFNGLGENVERTEXARRAYSPROC						glGenVertexArrays = 0;
PFNGLBINDVERTEXARRAYPROC						glBindVertexArray = 0;
PFNGLDELETEVERTEXARRAYSPROC						glDeleteVertexArrays = 0;
PFNGLCLEARBUFFERFVPROC							glClearBufferfv = 0;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC	glGetFramebufferAttachmentParameteriv = 0;
PFNGLGETUNIFORMBLOCKINDEXPROC					glGetUniformBlockIndex = 0;
PFNGLUNIFORMBLOCKBINDINGPROC					glUniformBlockBinding = 0;
PFNGLENABLEIPROC								glEnablei = 0;
PFNGLDISABLEIPROC								glDisablei = 0;
PFNGLBINDBUFFERBASEPROC							glBindBufferBase = 0;
PFNGLBINDBUFFERRANGEPROC						glBindBufferRange = 0;
PFNGLDRAWELEMENTSBASEVERTEXPROC					glDrawElementsBaseVertex = 0;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC		glDrawElementsInstancedBaseVertex = 0;

PFNGLGETSTRINGIPROC								glGetStringi = 0;
PFNGLGETINTEGERI_VPROC							glGetIntegeri_v = 0;
PFNGLFENCESYNCPROC								glFenceSync = 0;
PFNGLWAITSYNCPROC								glWaitSync = 0;
PFNGLCLIENTWAITSYNCPROC							glClientWaitSync = 0;
PFNGLDELETESYNCPROC								glDeleteSync = 0;

// 3.3
PFNGLGENSAMPLERSPROC							glGenSamplers = 0;
PFNGLDELETESAMPLERSPROC							glDeleteSamplers = 0;
PFNGLSAMPLERPARAMETERFPROC						glSamplerParameterf = 0;
PFNGLSAMPLERPARAMETERFVPROC						glSamplerParameterfv = 0;
PFNGLSAMPLERPARAMETERIPROC						glSamplerParameteri = 0;
PFNGLSAMPLERPARAMETERIVPROC						glSamplerParameteriv = 0;
PFNGLBINDSAMPLERPROC							glBindSampler = 0;
PFNGLVERTEXATTRIBDIVISORPROC					glVertexAttribDivisor = 0;

// 4.0
PFNGLGENTRANSFORMFEEDBACKSPROC					glGenTransformFeedbacks = 0;
PFNGLDELETETRANSFORMFEEDBACKSPROC				glDeleteTransformFeedbacks = 0;
PFNGLBINDTRANSFORMFEEDBACKPROC					glBindTransformFeedback = 0;
PFNGLDRAWTRANSFORMFEEDBACKPROC					glDrawTransformFeedback = 0;
PFNGLBEGINTRANSFORMFEEDBACKPROC					glBeginTransformFeedback = 0;
PFNGLENDTRANSFORMFEEDBACKPROC					glEndTransformFeedback = 0;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC				glTransformFeedbackVaryings = 0;
PFNGLPATCHPARAMETERIPROC						glPatchParameteri = 0;
PFNGLPATCHPARAMETERFVPROC						glPatchParameterfv = 0;
PFNGLUNIFORMSUBROUTINESUIVPROC					glUniformSubroutinesuiv = 0;
PFNGLBLENDEQUATIONIPROC							glBlendEquationi = 0;
PFNGLBLENDFUNCIPROC								glBlendFunci = 0;
PFNGLBLENDEQUATIONSEPARATEIPROC					glBlendEquationSeparatei = 0;
PFNGLBLENDFUNCSEPARATEIPROC						glBlendFuncSeparatei = 0;

// 4.1
PFNGLCREATESHADERPROGRAMVPROC					glCreateShaderProgramv = 0;
PFNGLPROGRAMPARAMETERIPROC						glProgramParameteri = 0;
PFNGLGENPROGRAMPIPELINESPROC					glGenProgramPipelines = 0;
PFNGLDELETEPROGRAMPIPELINESPROC					glDeleteProgramPipelines = 0;
PFNGLBINDPROGRAMPIPELINEPROC					glBindProgramPipeline = 0;
PFNGLUSEPROGRAMSTAGESPROC						glUseProgramStages = 0;
PFNGLVALIDATEPROGRAMPIPELINEPROC				glValidateProgramPipeline = 0;
PFNGLGETPROGRAMPIPELINEIVPROC					glGetProgramPipelineiv = 0;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC				glGetProgramPipelineInfoLog = 0;

PFNGLPROGRAMUNIFORM1IPROC						glProgramUniform1i = 0;
PFNGLPROGRAMUNIFORM1UIPROC						glProgramUniform1ui = 0;
PFNGLPROGRAMUNIFORM4UIPROC						glProgramUniform4ui = 0;
PFNGLPROGRAMUNIFORM4UIVPROC						glProgramUniform4uiv = 0;
PFNGLPROGRAMUNIFORM1FPROC						glProgramUniform1f = 0;
PFNGLPROGRAMUNIFORM2FVPROC						glProgramUniform2fv = 0;
PFNGLPROGRAMUNIFORM3FPROC						glProgramUniform3f = 0;
PFNGLPROGRAMUNIFORM4FVPROC						glProgramUniform4fv = 0;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC				glProgramUniformMatrix4fv = 0;

// 4.2
PFNGLTEXSTORAGE2DPROC									glTexStorage2D = 0;
PFNGLTEXSTORAGE3DPROC									glTexStorage3D = 0;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC	glDrawElementsInstancedBaseVertexBaseInstance = 0;

// 4.3
PFNGLDISPATCHCOMPUTEPROC						glDispatchCompute = 0;
PFNGLDISPATCHCOMPUTEINDIRECTPROC				glDispatchComputeIndirect = 0;
PFNGLBINDIMAGETEXTUREPROC						glBindImageTexture = 0;
PFNGLMEMORYBARRIERPROC							glMemoryBarrier = 0;
PFNGLMEMORYBARRIERBYREGIONPROC					glMemoryBarrierByRegion = 0;
PFNGLGETPROGRAMBINARYPROC						glGetProgramBinary = 0;
PFNGLGETPROGRAMINTERFACEIVPROC					glGetProgramInterfaceiv = 0;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC				glShaderStorageBlockBinding = 0;
PFNGLMULTIDRAWELEMENTSINDIRECTPROC				glMultiDrawElementsIndirect = 0;

PFNGLVERTEXATTRIBBINDINGPROC					glVertexAttribBinding = 0;
PFNGLVERTEXATTRIBFORMATPROC						glVertexAttribFormat = 0;
PFNGLVERTEXBINDINGDIVISORPROC					glVertexBindingDivisor = 0;
PFNGLBINDVERTEXBUFFERPROC						glBindVertexBuffer = 0;

PFNGLDEBUGMESSAGECONTROLPROC					glDebugMessageControl = 0;
PFNGLDEBUGMESSAGECALLBACKPROC					glDebugMessageCallback = 0;
PFNGLGETDEBUGMESSAGELOGPROC						glGetDebugMessageLog = 0;

PFNGLGETPROGRAMRESOURCENAMEPROC					glGetProgramResourceName = 0;
PFNGLGETPROGRAMRESOURCEINDEXPROC				glGetProgramResourceIndex = 0;
PFNGLGETPROGRAMRESOURCEIVPROC					glGetProgramResourceiv = 0;

// 4.4
PFNGLBUFFERSTORAGEPROC							glBufferStorage = 0;
PFNGLBINDVERTEXBUFFERSPROC						glBindVertexBuffers = 0;
PFNGLCLEARTEXIMAGEPROC							glClearTexImage = 0;
PFNGLCLEARTEXSUBIMAGEPROC						glClearTexSubImage = 0;

// 4.6
PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC			glMultiDrawElementsIndirectCount = 0;

// WGL specific
PFNWGLSWAPINTERVALFARPROC						wglSwapInterval = 0;
WGLCREATECONTEXTATTRIBSARBPROC					wglCreateContextAttribs = 0;
WGLGETEXTENSIONSSTRINGARBPROC					wglGetExtensionsString = 0;
WGLGETPIXELFORMATATTRIBIVARBPROC				wglGetPixelFormatAttribiv = 0;
WGLGETPIXELFORMATATTRIBFVARBPROC				wglGetPixelFormatAttribfv = 0;
WGLCHOOSEPIXELFORMATARBPROC						wglChoosePixelFormat = 0;
#endif

// --- Class implementation ---------------------------------------------------

uint16_t GLExtensions::GLSLVersion = 0;
uint16_t GLExtensions::GLVersion = 0;
int32_t GLExtensions::MaxUniformBlockSize = 0;
int32_t GLExtensions::UniformBufferOffsetAlignment = 0;

std::string GLExtensions::GLVendor;
std::string GLExtensions::GLRenderer;
std::string GLExtensions::GLAPIVersion;

bool GLExtensions::ARB_vertex_buffer_object = false;
bool GLExtensions::ARB_vertex_program = false;
bool GLExtensions::ARB_fragment_program = false;
bool GLExtensions::ARB_shader_objects = false;
bool GLExtensions::ARB_texture_float = false;
bool GLExtensions::ARB_texture_non_power_of_two = false;
bool GLExtensions::ARB_texture_rg = false;
bool GLExtensions::ARB_texture_compression = false;
bool GLExtensions::ARB_texture_multisample = false;
bool GLExtensions::ARB_draw_buffers = false;
bool GLExtensions::ARB_vertex_array_object = false;
bool GLExtensions::ARB_geometry_shader4 = false;
bool GLExtensions::ARB_tessellation_shader = false;
bool GLExtensions::ARB_compute_shader = false;
bool GLExtensions::ARB_shader_image_load_store = false;
bool GLExtensions::ARB_shader_storage_buffer_object = false;
bool GLExtensions::ARB_shader_atomic_counters = false;
bool GLExtensions::ARB_debug_output = false;
bool GLExtensions::ARB_multi_draw_indirect = false;
bool GLExtensions::ARB_indirect_parameters = false;
bool GLExtensions::ARB_shader_group_vote = false;

bool GLExtensions::EXT_texture_compression_s3tc = false;
bool GLExtensions::EXT_texture_cube_map = false;
bool GLExtensions::EXT_framebuffer_object = false;
bool GLExtensions::EXT_framebuffer_sRGB = false;
bool GLExtensions::EXT_texture_sRGB = false;
bool GLExtensions::EXT_framebuffer_multisample = false;
bool GLExtensions::EXT_framebuffer_blit = false;
bool GLExtensions::EXT_packed_depth_stencil = false;

#ifdef _WIN32
bool GLExtensions::WGL_EXT_swap_control = false;
bool GLExtensions::WGL_ARB_pixel_format = false;
bool GLExtensions::WGL_ARB_create_context = false;
bool GLExtensions::WGL_ARB_create_context_profile = false;

bool wIsSupported (const char* name, HDC hdc)
{
	const char *ext = 0, *start;
	const char *loc, *term;

	loc = strchr (name, ' ');

	if (loc || *name == '\0')
		return false;

	if (!wglGetExtensionsString)
		wglGetExtensionsString = (WGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress ("wglGetExtensionsStringARB");

	if( !wglGetExtensionsString )
		return false;

	ext = (const char*)wglGetExtensionsString (hdc);
	start = ext;

	for( ;; ) {
		loc = strstr(start, name);

		if (loc == 0)
			break;

		term = loc + strlen (name);

		if (loc == start || *(loc - 1) == ' ') {
			if (*term == ' ' || *term == '\0')
				return true;
		}

		start = term;
	}

	return false;
}

#endif

GLExtensions::GLExtensions ()
{
}

void GLExtensions::QueryFeatures (void* hdc)
{
	if (GLVersion > 0)
		return;

	const char* glversion = (const char*)glGetString (GL_VERSION);
	uint16_t major, minor;
	bool isgles = false;

	if (0 == sscanf (glversion, "%1hu.%2hu %*s", &major, &minor)) {
		sscanf (glversion, "OpenGL ES %1hu %*s", &major);

		minor = 0;
		isgles = true;
	}

	GLVersion = MAKE_VERSION(major, minor);
	
	GLAPIVersion = glversion;
	GLVendor = (const char*)glGetString (GL_VENDOR);
	GLRenderer = (const char*)glGetString (GL_RENDERER);

	bool coreprofile = (GLVersion >= GL_3_2);
	
#ifdef _WIN32
	if (hdc) {
		WGL_EXT_swap_control				= wIsSupported ("WGL_EXT_swap_control", (HDC)hdc);
		WGL_ARB_pixel_format				= wIsSupported ("WGL_ARB_pixel_format", (HDC)hdc);
		WGL_ARB_create_context				= wIsSupported ("WGL_ARB_create_context", (HDC)hdc);
		WGL_ARB_create_context_profile		= wIsSupported ("WGL_ARB_create_context_profile", (HDC)hdc);
	}

	if (WGL_ARB_pixel_format) {
		wglGetPixelFormatAttribiv			= (WGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress ("wglGetPixelFormatAttribivARB");
		wglGetPixelFormatAttribfv			= (WGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress ("wglGetPixelFormatAttribfvARB");
		wglChoosePixelFormat				= (WGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress ("wglChoosePixelFormatARB");
	}

	if (WGL_ARB_create_context && WGL_ARB_create_context_profile)
		wglCreateContextAttribs				= (WGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress ("wglCreateContextAttribsARB");

	if (WGL_EXT_swap_control)
		wglSwapInterval						= (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress ("wglSwapIntervalEXT");
#endif

	if (coreprofile) {
#ifdef _WIN32
		GET_ADDRESS(glGetStringi, PFNGLGETSTRINGIPROC, "");
#endif

		ARB_vertex_buffer_object			= true;
		ARB_vertex_program					= true;
		ARB_fragment_program				= true;
		ARB_shader_objects					= true;
		ARB_texture_float					= true;
		ARB_texture_non_power_of_two		= true;
		ARB_texture_rg						= true;
		ARB_texture_compression				= true;
		ARB_texture_multisample				= true;
		ARB_draw_buffers					= true;
		ARB_vertex_array_object				= true;

		EXT_framebuffer_object				= true;
		EXT_texture_cube_map				= true;
		EXT_framebuffer_sRGB				= true;
		EXT_texture_sRGB					= true;
		EXT_texture_compression_s3tc		= true;
		EXT_framebuffer_multisample			= true;
		EXT_framebuffer_blit				= true;
		EXT_packed_depth_stencil			= true;

		ARB_debug_output					= IsSupported ("GL_ARB_debug_output");
		ARB_geometry_shader4				= IsSupported ("GL_ARB_geometry_shader4");
		ARB_tessellation_shader				= IsSupported ("GL_ARB_tessellation_shader");
		ARB_compute_shader					= IsSupported ("GL_ARB_compute_shader");
		ARB_shader_image_load_store			= IsSupported ("GL_ARB_shader_image_load_store");
		ARB_shader_storage_buffer_object	= IsSupported ("GL_ARB_shader_storage_buffer_object");
		ARB_shader_atomic_counters			= IsSupported ("GL_ARB_shader_atomic_counters");
		ARB_multi_draw_indirect				= IsSupported ("GL_ARB_multi_draw_indirect");
		ARB_indirect_parameters				= IsSupported ("GL_ARB_indirect_parameters");
		ARB_shader_group_vote				= IsSupported ("GL_ARB_shader_group_vote");

		glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, &MaxUniformBlockSize);
		glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffsetAlignment);
	} else {
		ARB_vertex_buffer_object			= IsSupported ("GL_ARB_vertex_buffer_object");
		ARB_vertex_program					= IsSupported ("GL_ARB_vertex_program");
		ARB_fragment_program				= IsSupported ("GL_ARB_fragment_program");
		ARB_shader_objects					= IsSupported ("GL_ARB_shader_objects");
		ARB_texture_float					= IsSupported ("GL_ARB_texture_float");
		ARB_texture_non_power_of_two		= IsSupported ("GL_ARB_texture_non_power_of_two");
		ARB_texture_rg						= IsSupported ("GL_ARB_texture_rg");
		ARB_texture_compression				= IsSupported ("GL_ARB_texture_compression");
		ARB_texture_multisample				= IsSupported ("GL_ARB_texture_multisample");
		ARB_draw_buffers					= IsSupported ("GL_ARB_draw_buffers");
		ARB_vertex_array_object				= IsSupported ("GL_ARB_vertex_array_object");

		EXT_framebuffer_object				= IsSupported ("GL_EXT_framebuffer_object");
		EXT_framebuffer_sRGB				= IsSupported ("GL_EXT_framebuffer_sRGB");
		EXT_framebuffer_multisample			= IsSupported ("GL_EXT_framebuffer_multisample");
		EXT_framebuffer_blit				= IsSupported ("GL_EXT_framebuffer_blit");
		EXT_texture_sRGB					= IsSupported ("GL_EXT_texture_sRGB");
		EXT_texture_compression_s3tc		= IsSupported ("GL_EXT_texture_compression_s3tc");
		EXT_texture_cube_map				= IsSupported ("GL_EXT_texture_cube_map");
		EXT_packed_depth_stencil			= IsSupported ("GL_EXT_packed_depth_stencil");

		if (!EXT_framebuffer_sRGB)
			EXT_framebuffer_sRGB			= IsSupported ("GL_ARB_framebuffer_sRGB");

		if (!EXT_texture_cube_map)
			EXT_texture_cube_map			= IsSupported ("GL_ARB_texture_cube_map");
		
		if (!ARB_texture_float)
			ARB_texture_float				= IsSupported ("GL_APPLE_float_pixels");
	}

#ifdef _WIN32
	GET_ADDRESS(glActiveTexture, PFNGLACTIVETEXTUREARBPROC, "glActiveTexture");
	GET_ADDRESS(glClientActiveTexture, PFNGLCLIENTACTIVETEXTUREARBPROC, "glClientActiveTexture");
	GET_ADDRESS(glGenerateMipmap, PFNGLGENERATEMIPMAPEXTPROC, "glGenerateMipmapEXT");
	GET_ADDRESS(glMapBuffer, PFNGLMAPBUFFERARBPROC, "glMapBufferARB");
	GET_ADDRESS(glUnmapBuffer, PFNGLUNMAPBUFFERARBPROC, "glUnmapBufferARB");
	GET_ADDRESS(glBlendEquation, PFNGLBLENDEQUATIONPROC, "glBlendEquationARB");
	GET_ADDRESS(glBlendEquationSeparate, PFNGLBLENDEQUATIONSEPARATEPROC, "glBlendEquationSeparateARB");
	GET_ADDRESS(glBlendFuncSeparate, PFNGLBLENDFUNCSEPARATEPROC, "glBlendFuncSeparateARB");
	GET_ADDRESS(glStencilFuncSeparate, PFNGLSTENCILFUNCSEPARATEPROC, "glStencilFuncSeparateARB");
	GET_ADDRESS(glStencilMaskSeparate, PFNGLSTENCILMASKSEPARATEPROC, "glStencilMaskSeparateARB");
	GET_ADDRESS(glStencilOpSeparate, PFNGLSTENCILOPSEPARATEPROC, "glStencilOpSeparateARB");
	GET_ADDRESS(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC, "glGetVertexAttribivARB");
	GET_ADDRESS(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC, "glGetVertexAttribfvARB");

	GET_ADDRESS(glGenQueries, PFNGLGENQUERIESPROC, "glGenQueriesARB");
	GET_ADDRESS(glDeleteQueries, PFNGLDELETEQUERIESPROC, "glDeleteQueriesARB");
	GET_ADDRESS(glBeginQuery, PFNGLBEGINQUERYPROC, "glBeginQueryARB");
	GET_ADDRESS(glEndQuery, PFNGLENDQUERYPROC, "glEndQueryARB");
	GET_ADDRESS(glGetQueryObjectuiv, PFNGLGETQUERYOBJECTUIVPROC, "glGetQueryObjectuivARB");
	GET_ADDRESS(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC, "glGetBufferParameterivARB");

	GET_ADDRESS(glMultiDrawElements, PFNGLMULTIDRAWELEMENTSPROC, "glMultiDrawElementsARB");

	if (ARB_texture_compression) {
		GET_ADDRESS(glCompressedTexImage3D, PFNGLCOMPRESSEDTEXIMAGE3DPROC, "glCompressedTexImage3D");
		GET_ADDRESS(glCompressedTexImage2D, PFNGLCOMPRESSEDTEXIMAGE2DPROC, "glCompressedTexImage2D");
		GET_ADDRESS(glCompressedTexImage1D, PFNGLCOMPRESSEDTEXIMAGE1DPROC, "glCompressedTexImage1D");
	}

	if (ARB_draw_buffers)
		GET_ADDRESS(glDrawBuffers, PFNGLDRAWBUFFERSARBPROC, "glDrawBuffers");

	if (ARB_vertex_array_object) {
		GET_ADDRESS(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC, "glGenVertexArrays");
		GET_ADDRESS(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC, "glBindVertexArray");
		GET_ADDRESS(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC, "glDeleteVertexArrays");
	}

	if (ARB_vertex_buffer_object) {
		GET_ADDRESS(glGetBufferSubData, PFNGLGETBUFFERSUBDATAARBPROC, "glGetBufferSubData");
		GET_ADDRESS(glDrawRangeElements, PFNGLDRAWRANGEELEMENTSPROC, "glDrawRangeElements");

		GET_ADDRESS(glDeleteBuffers, PFNGLDELETEBUFFERSARBPROC, "glDeleteBuffersARB");
		GET_ADDRESS(glBindBuffer, PFNGLBINDBUFFERARBPROC, "glBindBufferARB");
		GET_ADDRESS(glGenBuffers, PFNGLGENBUFFERSARBPROC, "glGenBuffersARB");
		GET_ADDRESS(glBufferData, PFNGLBUFFERDATAARBPROC, "glBufferDataARB");
		GET_ADDRESS(glBufferSubData, PFNGLBUFFERSUBDATAARBPROC, "glBufferSubDataARB");
	}

	if (EXT_framebuffer_object) {
		GET_ADDRESS(glGenFramebuffers, PFNGLGENFRAMEBUFFERSEXTPROC, "glGenFramebuffersEXT");
		GET_ADDRESS(glGenRenderbuffers, PFNGLGENRENDERBUFFERSEXTPROC, "glGenRenderbuffersEXT");
		GET_ADDRESS(glBindFramebuffer, PFNGLBINDFRAMEBUFFEREXTPROC, "glBindFramebufferEXT");
		GET_ADDRESS(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, "glFramebufferTexture2DEXT");
		GET_ADDRESS(glBindRenderbuffer, PFNGLBINDRENDERBUFFEREXTPROC, "glBindRenderbufferEXT");
		GET_ADDRESS(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEEXTPROC, "glRenderbufferStorageEXT");
		GET_ADDRESS(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, "glFramebufferRenderbufferEXT");
		GET_ADDRESS(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, "glCheckFramebufferStatusEXT");
		GET_ADDRESS(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSEXTPROC, "glDeleteFramebuffersEXT");
		GET_ADDRESS(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSEXTPROC, "glDeleteRenderbuffersEXT");
	}

	if (EXT_framebuffer_multisample)
		GET_ADDRESS(glRenderbufferStorageMultisample, PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC, "glRenderbufferStorageMultisampleEXT");

	if (EXT_framebuffer_blit)
		GET_ADDRESS(glBlitFramebuffer, PFNGLBLITFRAMEBUFFEREXTPROC, "glBlitFramebufferEXT");

	if (ARB_shader_objects) {
		GET_ADDRESS(glCreateProgram, PFNGLCREATEPROGRAMOBJECTARBPROC, "glCreateProgramObjectARB");
		GET_ADDRESS(glCreateShader, PFNGLCREATESHADEROBJECTARBPROC, "glCreateShaderObjectARB");
		GET_ADDRESS(glShaderSource, PFNGLSHADERSOURCEARBPROC, "glShaderSourceARB");
		GET_ADDRESS(glCompileShader, PFNGLCOMPILESHADERARBPROC, "glCompileShaderARB");
		GET_ADDRESS(glAttachShader, PFNGLATTACHOBJECTARBPROC, "glAttachObjectARB");
		GET_ADDRESS(glDetachShader, PFNGLDETACHOBJECTARBPROC, "glDetachObjectARB");
		GET_ADDRESS(glLinkProgram, PFNGLLINKPROGRAMARBPROC, "glLinkProgramARB");
		GET_ADDRESS(glDeleteProgram, PFNGLDELETEOBJECTARBPROC, "glDeleteObjectARB");
		GET_ADDRESS(glDeleteShader, PFNGLDELETEOBJECTARBPROC, "glDeleteObjectARB");
		GET_ADDRESS(glUseProgram, PFNGLUSEPROGRAMOBJECTARBPROC, "glUseProgramObjectARB");

		GET_ADDRESS(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVARBPROC, "glUniformMatrix2fvARB");
		GET_ADDRESS(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVARBPROC, "glUniformMatrix3fvARB");
		GET_ADDRESS(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVARBPROC, "glUniformMatrix4fvARB");
		GET_ADDRESS(glUniform1i, PFNGLUNIFORM1IARBPROC, "glUniform1iARB");
		GET_ADDRESS(glUniform4ui, PFNGLUNIFORM4UIPROC, "glUniform4uiARB");
		GET_ADDRESS(glUniform4uiv, PFNGLUNIFORM4UIVPROC, "glUniform4uivARB");
		GET_ADDRESS(glUniform1f, PFNGLUNIFORM1FARBPROC, "glUniform1fARB");
		GET_ADDRESS(glUniform2f, PFNGLUNIFORM2FARBPROC, "glUniform2fARB");
		GET_ADDRESS(glUniform3f, PFNGLUNIFORM3FARBPROC, "glUniform3fARB");
		GET_ADDRESS(glUniform4f, PFNGLUNIFORM4FARBPROC, "glUniform4fARB");
		GET_ADDRESS(glUniform1fv, PFNGLUNIFORM1FVARBPROC, "glUniform1fvARB");
		GET_ADDRESS(glUniform2fv, PFNGLUNIFORM2FVARBPROC, "glUniform2fvARB");
		GET_ADDRESS(glUniform3fv, PFNGLUNIFORM3FVARBPROC, "glUniform3fvARB");
		GET_ADDRESS(glUniform4fv, PFNGLUNIFORM4FVARBPROC, "glUniform4fvARB");

		GET_ADDRESS(glGetProgramiv, PFNGLGETOBJECTPARAMETERIVARBPROC, "glGetObjectParameterivARB");
		GET_ADDRESS(glGetShaderiv, PFNGLGETOBJECTPARAMETERIVARBPROC, "glGetObjectParameterivARB");
		GET_ADDRESS(glGetActiveUniform, PFNGLGETACTIVEUNIFORMARBPROC, "glGetActiveUniformARB");
		GET_ADDRESS(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC, "glGetActiveAttribARB");
		GET_ADDRESS(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC, "glGetAttribLocationARB");
		GET_ADDRESS(glGetShaderInfoLog, PFNGLGETINFOLOGARBPROC, "glGetInfoLogARB");
		GET_ADDRESS(glGetProgramInfoLog, PFNGLGETINFOLOGARBPROC, "glGetInfoLogARB");
		GET_ADDRESS(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONARBPROC, "glGetUniformLocationARB");

		GET_ADDRESS(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONARBPROC, "glBindAttribLocationARB");
		GET_ADDRESS(glBindFragDataLocation, PFNGLBINDFRAGDATALOCATIONPROC, "glBindFragDataLocation");

		GET_ADDRESS(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYARBPROC, "glEnableVertexAttribArrayARB");
		GET_ADDRESS(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYARBPROC, "glDisableVertexAttribArrayARB");
		GET_ADDRESS(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERARBPROC, "glVertexAttribPointerARB");
	}

	// core profile only
	if (coreprofile) {
		GET_ADDRESS(glMapBufferRange, PFNGLMAPBUFFERRANGEPROC, "");
		GET_ADDRESS(glTexImage3D, PFNGLTEXIMAGE3DPROC, "");
		GET_ADDRESS(glTexImage2DMultisample, PFNGLTEXIMAGE2DMULTISAMPLEPROC, "");
		GET_ADDRESS(glTexSubImage3D, PFNGLTEXSUBIMAGE3DPROC, "");

		GET_ADDRESS(glGetIntegeri_v, PFNGLGETINTEGERI_VPROC, "");
		GET_ADDRESS(glFenceSync, PFNGLFENCESYNCPROC, "");
		GET_ADDRESS(glWaitSync, PFNGLWAITSYNCPROC, "");
		GET_ADDRESS(glClientWaitSync, PFNGLCLIENTWAITSYNCPROC, "");
		GET_ADDRESS(glDeleteSync, PFNGLDELETESYNCPROC, "");

		GET_ADDRESS(glGetProgramBinary, PFNGLGETPROGRAMBINARYPROC, "");
		GET_ADDRESS(glClearBufferfv, PFNGLCLEARBUFFERFVPROC, "");
		GET_ADDRESS(glGetFramebufferAttachmentParameteriv, PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC, "");
		GET_ADDRESS(glFramebufferTextureLayer, PFNGLFRAMEBUFFERTEXTURELAYERPROC, "");
		GET_ADDRESS(glGetUniformBlockIndex, PFNGLGETUNIFORMBLOCKINDEXPROC, "");
		GET_ADDRESS(glUniformBlockBinding, PFNGLUNIFORMBLOCKBINDINGPROC, "");
		GET_ADDRESS(glEnablei, PFNGLENABLEIPROC, "");
		GET_ADDRESS(glDisablei, PFNGLDISABLEIPROC, "");
		GET_ADDRESS(glBindBufferBase, PFNGLBINDBUFFERBASEPROC, "");
		GET_ADDRESS(glBindBufferRange, PFNGLBINDBUFFERRANGEPROC, "");
		GET_ADDRESS(glDrawElementsBaseVertex, PFNGLDRAWELEMENTSBASEVERTEXPROC, "");
		GET_ADDRESS(glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCEDPROC, "");
		GET_ADDRESS(glDrawElementsInstanced, PFNGLDRAWELEMENTSINSTANCEDPROC, "");
		GET_ADDRESS(glDrawElementsInstancedBaseVertex, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC, "");
		GET_ADDRESS(glFlushMappedBufferRange, PFNGLFLUSHMAPPEDBUFFERRANGEPROC, "");
		GET_ADDRESS(glColorMaski, PFNGLCOLORMASKIPROC, "");
		GET_ADDRESS(glGetActiveUniformBlockiv, PFNGLGETACTIVEUNIFORMBLOCKIVPROC, "");
		GET_ADDRESS(glGetActiveUniformBlockName, PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC, "");
		GET_ADDRESS(glTexBuffer, PFNGLTEXBUFFERPROC, "");
		GET_ADDRESS(glPrimitiveRestartIndex, PFNGLPRIMITIVERESTARTINDEXPROC, "");
	}

	if (GLVersion >= GL_3_3) {
		GET_ADDRESS(glGenSamplers, PFNGLGENSAMPLERSPROC, "");
		GET_ADDRESS(glDeleteSamplers, PFNGLDELETESAMPLERSPROC, "");
		GET_ADDRESS(glSamplerParameterf, PFNGLSAMPLERPARAMETERFPROC, "");
		GET_ADDRESS(glSamplerParameterfv, PFNGLSAMPLERPARAMETERFVPROC, "");
		GET_ADDRESS(glSamplerParameteri, PFNGLSAMPLERPARAMETERIPROC, "");
		GET_ADDRESS(glSamplerParameteriv, PFNGLSAMPLERPARAMETERIVPROC, "");
		GET_ADDRESS(glBindSampler, PFNGLBINDSAMPLERPROC, "");
		GET_ADDRESS(glVertexAttribDivisor, PFNGLVERTEXATTRIBDIVISORPROC, "");
	}

	if (GLVersion >= GL_4_0) {
		GET_ADDRESS(glBlendEquationi, PFNGLBLENDEQUATIONIPROC, "");
		GET_ADDRESS(glBlendFunci, PFNGLBLENDFUNCIPROC, "");
		GET_ADDRESS(glBlendEquationSeparatei, PFNGLBLENDEQUATIONSEPARATEIPROC, "");
		GET_ADDRESS(glBlendFuncSeparatei, PFNGLBLENDFUNCSEPARATEIPROC, "");

		GET_ADDRESS(glGenTransformFeedbacks, PFNGLGENTRANSFORMFEEDBACKSPROC, "");
		GET_ADDRESS(glDeleteTransformFeedbacks, PFNGLDELETETRANSFORMFEEDBACKSPROC, "");
		GET_ADDRESS(glBindTransformFeedback, PFNGLBINDTRANSFORMFEEDBACKPROC, "");
		GET_ADDRESS(glDrawTransformFeedback, PFNGLDRAWTRANSFORMFEEDBACKPROC, "");
		GET_ADDRESS(glBeginTransformFeedback, PFNGLBEGINTRANSFORMFEEDBACKPROC, "");
		GET_ADDRESS(glEndTransformFeedback, PFNGLENDTRANSFORMFEEDBACKPROC, "");
		GET_ADDRESS(glTransformFeedbackVaryings, PFNGLTRANSFORMFEEDBACKVARYINGSPROC, "");

		GET_ADDRESS(glUniformSubroutinesuiv, PFNGLUNIFORMSUBROUTINESUIVPROC, "");
	}

	if (GLVersion >= GL_4_1) {
		GET_ADDRESS(glCreateShaderProgramv, PFNGLCREATESHADERPROGRAMVPROC, "");
		GET_ADDRESS(glProgramParameteri, PFNGLPROGRAMPARAMETERIPROC, "");
		GET_ADDRESS(glGenProgramPipelines, PFNGLGENPROGRAMPIPELINESPROC, "");
		GET_ADDRESS(glDeleteProgramPipelines, PFNGLDELETEPROGRAMPIPELINESPROC, "");
		GET_ADDRESS(glBindProgramPipeline, PFNGLBINDPROGRAMPIPELINEPROC, "");
		GET_ADDRESS(glUseProgramStages, PFNGLUSEPROGRAMSTAGESPROC, "");
		GET_ADDRESS(glValidateProgramPipeline, PFNGLVALIDATEPROGRAMPIPELINEPROC, "");
		GET_ADDRESS(glGetProgramPipelineiv, PFNGLGETPROGRAMPIPELINEIVPROC, "");
		GET_ADDRESS(glGetProgramPipelineInfoLog, PFNGLGETPROGRAMPIPELINEINFOLOGPROC, "");

		GET_ADDRESS(glProgramUniform1i, PFNGLPROGRAMUNIFORM1IPROC, "");
		GET_ADDRESS(glProgramUniform1ui, PFNGLPROGRAMUNIFORM1UIPROC, "");
		GET_ADDRESS(glProgramUniform4ui, PFNGLPROGRAMUNIFORM4UIPROC, "");
		GET_ADDRESS(glProgramUniform4uiv, PFNGLPROGRAMUNIFORM4UIVPROC, "");
		GET_ADDRESS(glProgramUniform1f, PFNGLPROGRAMUNIFORM1FPROC, "");
		GET_ADDRESS(glProgramUniform2fv, PFNGLPROGRAMUNIFORM2FVPROC, "");
		GET_ADDRESS(glProgramUniform3f, PFNGLPROGRAMUNIFORM3FPROC, "");
		GET_ADDRESS(glProgramUniform4fv, PFNGLPROGRAMUNIFORM4FVPROC, "");
		GET_ADDRESS(glProgramUniformMatrix4fv, PFNGLPROGRAMUNIFORMMATRIX4FVPROC, "");
	}

	if (GLVersion >= GL_4_2) {
		GET_ADDRESS(glDrawElementsInstancedBaseVertexBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC, "");
		GET_ADDRESS(glTexStorage2D, PFNGLTEXSTORAGE2DPROC, "");
		GET_ADDRESS(glTexStorage3D, PFNGLTEXSTORAGE3DPROC, "");
	}

	if (GLVersion >= GL_4_3) {
		GET_ADDRESS(glVertexAttribBinding, PFNGLVERTEXATTRIBBINDINGPROC, "");
		GET_ADDRESS(glVertexAttribFormat, PFNGLVERTEXATTRIBFORMATPROC, "");
		GET_ADDRESS(glVertexBindingDivisor, PFNGLVERTEXBINDINGDIVISORPROC, "");
		GET_ADDRESS(glBindVertexBuffer, PFNGLBINDVERTEXBUFFERPROC, "");
		GET_ADDRESS(glMultiDrawElementsIndirect, PFNGLMULTIDRAWELEMENTSINDIRECTPROC, "");

		GET_ADDRESS(glGetProgramResourceName, PFNGLGETPROGRAMRESOURCENAMEPROC, "");
		GET_ADDRESS(glGetProgramResourceIndex, PFNGLGETPROGRAMRESOURCEINDEXPROC, "");
		GET_ADDRESS(glGetProgramResourceiv, PFNGLGETPROGRAMRESOURCEIVPROC, "");
	}

	if (GLVersion >= GL_4_4) {
		GET_ADDRESS(glBufferStorage, PFNGLBUFFERSTORAGEPROC, "");
		GET_ADDRESS(glBindVertexBuffers, PFNGLBINDVERTEXBUFFERSPROC, "");
		GET_ADDRESS(glClearTexImage, PFNGLCLEARTEXIMAGEPROC, "");
		GET_ADDRESS(glClearTexSubImage, PFNGLCLEARTEXSUBIMAGEPROC, "");
	}

	if (GLVersion >= GL_4_6) {
		GET_ADDRESS(glMultiDrawElementsIndirectCount, PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC, "");
	} else if (ARB_indirect_parameters) {
		GET_ADDRESS(glMultiDrawElementsIndirectCount, PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC, "glMultiDrawElementsIndirectCountARB");
	}

	if (ARB_tessellation_shader) {
		GET_ADDRESS(glPatchParameteri, PFNGLPATCHPARAMETERIPROC, "");
		GET_ADDRESS(glPatchParameterfv, PFNGLPATCHPARAMETERFVPROC, "");
	}

	if (ARB_compute_shader) {
		GET_ADDRESS(glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC, "");
		GET_ADDRESS(glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC, "");
		GET_ADDRESS(glMemoryBarrier, PFNGLMEMORYBARRIERPROC, "");
		GET_ADDRESS(glMemoryBarrierByRegion, PFNGLMEMORYBARRIERBYREGIONPROC, "");
	}

	if (ARB_shader_image_load_store) {
		GET_ADDRESS(glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC, "");
	}

	if (ARB_shader_storage_buffer_object) {
		GET_ADDRESS(glGetProgramInterfaceiv, PFNGLGETPROGRAMINTERFACEIVPROC, "");
		GET_ADDRESS(glShaderStorageBlockBinding, PFNGLSHADERSTORAGEBLOCKBINDINGPROC, "");
	}

	if (ARB_debug_output) {
		GET_ADDRESS(glDebugMessageControl, PFNGLDEBUGMESSAGECONTROLPROC, "");
		GET_ADDRESS(glDebugMessageCallback, PFNGLDEBUGMESSAGECALLBACKPROC, "");
		GET_ADDRESS(glGetDebugMessageLog, PFNGLGETDEBUGMESSAGELOGPROC, "");
	}
#endif
		
	if (ARB_shader_objects) {
		const char* glslversion = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
		uint16_t major, minor;

		sscanf (glslversion, "%1hu.%2hu %*s", &major, &minor);
		GLSLVersion = MAKE_VERSION(major, minor);
	} else {
		GLSLVersion = 0;
	}
}

bool GLExtensions::IsSupported (const char* name)
{
	const char* ext = 0;
	const char* loc;

	loc = strchr (name, ' ');

	if (loc || *name == '\0')
		return false;
		
#ifdef _WIN32
	if (glGetStringi) {
#endif
		GLint numext = 0;

		glGetIntegerv (GL_NUM_EXTENSIONS, &numext);

		for (GLint i = 0; i < numext; ++i) {
			ext = (const char*)glGetStringi (GL_EXTENSIONS, i);

			if (0 == strcmp (ext, name))
				return true;
		}
#ifdef _WIN32
	}
#endif

	return false;
}
