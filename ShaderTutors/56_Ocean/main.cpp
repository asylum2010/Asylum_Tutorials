
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <random>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\spectatorcamera.h"
#include "..\Common\terrainquadtree.h"

// tweakables
#define DISP_MAP_SIZE		512					// 1024 max
#define MESH_SIZE			256					// [64, 256] (or calculate index count for other levels)
#define GRAV_ACCELERATION	9.81f				// m/s^2
#define PATCH_SIZE			20.0f				// m
#define FURTHEST_COVER		8					// full ocean size = PATCH_SIZE * (1 << FURTHEST_COVER)
#define MAX_COVERAGE		64.0f				// pixel limit for a distant patch to be rendered
#define WIND_DIRECTION		{ -0.4f, -0.9f }
#define WIND_SPEED			6.5f				// m/s
#define AMPLITUDE_CONSTANT	(0.45f * 1e-3f)		// for the (modified) Phillips spectrum

// helper macros
#define TITLE				"Shader sample 56: Ocean rendering"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

static const int IndexCounts[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	961920,		// 64x64
	3705084,	// 128x128
	14500728	// 256x256
};

Math::Color OceanColors[] = {
	{ 0.0056f, 0.0194f, 0.0331f, 1 },	// deep blue
	{ 0.1812f, 0.4678f, 0.5520f, 1 },	// carribbean
	{ 0.0000f, 0.2307f, 0.3613f, 1 },	// light blue
	{ 0.2122f, 0.6105f, 1.0000f, 1 },
	{ 0.0123f, 0.3613f, 0.6867f, 1 },
	{ 0.0000f, 0.0999f, 0.4508f, 1 },
	{ 0.0000f, 0.0331f, 0.1329f, 1 },
	{ 0.0000f, 0.0103f, 0.0331f, 1 }
};

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			oceanmesh		= nullptr;
OpenGLMesh*			skymesh			= nullptr;

OpenGLEffect*		debugeffect		= nullptr;
OpenGLEffect*		updatespectrum	= nullptr;
OpenGLEffect*		fourier_dft		= nullptr;	// bruteforce solution
OpenGLEffect*		fourier_fft		= nullptr;	// fast fourier transform
OpenGLEffect*		createdisp		= nullptr;	// displacement
OpenGLEffect*		creategrad		= nullptr;	// normal & jacobian
OpenGLEffect*		oceaneffect		= nullptr;
OpenGLEffect*		wireeffect		= nullptr;
OpenGLEffect*		skyeffect		= nullptr;

OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				initial			= 0;		// initial spectrum \tilde{h}_0
GLuint				frequencies		= 0;		// frequency \omega_i per wave vector
GLuint				updated[2]		= { 0 };	// updated spectra \tilde{h}(\mathbf{k},t) and \tilde{\mathbf{D}}(\mathbf{k},t) [reused for FT result]
GLuint				tempdata		= 0;		// intermediate data for FT
GLuint				displacement	= 0;		// displacement map
GLuint				gradients		= 0;		// normal & folding map
GLuint				perlintex		= 0;		// Perlin noise to remove tiling artifacts
GLuint				environment		= 0;
GLuint				debugvao		= 0;
GLuint				helptext		= 0;

TerrainQuadTree		tree;
SpectatorCamera		camera;
SpectatorCamera		debugcamera;

uint32_t			numlods			= 0;
int					currentcolor	= 7;
bool				use_fft			= true;
bool				use_debug		= false;
bool				drawtext		= true;

// forward declarations
extern void FFT_Test();

void FourierTransform(GLuint spectrum);
void GenerateLODLevels(OpenGLAttributeRange** subsettable, GLuint* numsubsets, uint32_t* idata);
GLuint GenerateBoundaryMesh(int deg_left, int deg_top, int deg_right, int deg_bottom, int levelsize, uint32_t* idata);

// static functions
static float Phillips(const Math::Vector2& k, const Math::Vector2& w, float V, float A)
{
	float L = (V * V) / GRAV_ACCELERATION;	// largest possible wave for wind speed V
	float l = L / 1000.0f;					// supress waves smaller than this

	float kdotw = Math::Vec2Dot(k, w);
	float k2 = Math::Vec2Dot(k, k);			// squared length of wave vector k

	// k^6 because k must be normalized
	float P_h = A * (expf(-1.0f / (k2 * L * L))) / (k2 * k2 * k2) * (kdotw * kdotw);

	if (kdotw < 0.0f) {
		// wave is moving against wind direction w
		P_h *= 0.07f;
	}

	return P_h * expf(-k2 * l * l);
}

static float Fresnel(float alpha, float n1, float n2)
{
	// air = 1.000293f, water = 1.33f
	float sina = sinf(alpha);
	float sing = sina * (n1 / n2);
	float cosa = cosf(alpha);
	float cosg = sqrtf(1.0f - sing * sing);

	float Rs = (n1 * cosa - n2 * cosg) / (n1 * cosa + n2 * cosg);
	float Rp = (n1 * cosg - n2 * cosa) / (n1 * cosg + n2 * cosa);

	Rs *= Rs;
	Rp *= Rp;

	return 0.5f * (Rs + Rp);
}

static GLuint CalcSubsetIndex(int level, int dL, int dR, int dB, int dT)
{
	// returns the subset index of the given LOD levels
	return 2 * (level * 3 * 3 * 3 * 3 + dL * 3 * 3 * 3 + dR * 3 * 3 + dB * 3 + dT);
}

// --- Sample impl ------------------------------------------------------------

bool InitScene()
{
	std::mt19937 gen;
	std::normal_distribution<> gaussian(0.0, 1.0);
	GLint maxanisotropy = 1;

	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	// test the modified FFT algorithm
	FFT_Test();

	// setup OpenGL
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxanisotropy);
	maxanisotropy = Math::Max(maxanisotropy, 2);

	// generate initial spectrum and frequencies
	Math::Vector2 k;
	float L = PATCH_SIZE;

	glGenTextures(1, &initial);
	glGenTextures(1, &frequencies);

	glBindTexture(GL_TEXTURE_2D, initial);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, DISP_MAP_SIZE + 1, DISP_MAP_SIZE + 1);

	glBindTexture(GL_TEXTURE_2D, frequencies);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, DISP_MAP_SIZE + 1, DISP_MAP_SIZE + 1);

	// n, m should be be in [-N / 2, N / 2]
	int start = DISP_MAP_SIZE / 2;

	// NOTE: in order to be symmetric, this must be (N + 1) x (N + 1) in size
	Math::Complex* h0data = new Math::Complex[(DISP_MAP_SIZE + 1) * (DISP_MAP_SIZE + 1)];
	float* wdata = new float[(DISP_MAP_SIZE + 1) * (DISP_MAP_SIZE + 1)];
	{
		Math::Vector2 w = WIND_DIRECTION;
		Math::Vector2 wn;
		float V = WIND_SPEED;
		float A = AMPLITUDE_CONSTANT;

		Math::Vec2Normalize(wn, w);

		for (int m = 0; m <= DISP_MAP_SIZE; ++m) {
			k.y = (Math::TWO_PI * (start - m)) / L;

			for (int n = 0; n <= DISP_MAP_SIZE; ++n) {
				k.x = (Math::TWO_PI * (start - n)) / L;
				
				int index = m * (DISP_MAP_SIZE + 1) + n;
				float sqrt_P_h = 0;

				if (k.x != 0.0f || k.y != 0.0f)
					sqrt_P_h = sqrtf(Phillips(k, wn, V, A));

				h0data[index].a = (float)(sqrt_P_h * gaussian(gen) * Math::ONE_OVER_SQRT_2);
				h0data[index].b = (float)(sqrt_P_h * gaussian(gen) * Math::ONE_OVER_SQRT_2);

				// dispersion relation \omega^2(k) = gk
				wdata[index] = sqrtf(GRAV_ACCELERATION * Math::Vec2Length(k));
			}
		}
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DISP_MAP_SIZE + 1, DISP_MAP_SIZE + 1, GL_RED, GL_FLOAT, wdata);

	glBindTexture(GL_TEXTURE_2D, initial);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DISP_MAP_SIZE + 1, DISP_MAP_SIZE + 1, GL_RG, GL_FLOAT, h0data);

	delete[] wdata;
	delete[] h0data;

	// create other spectrum textures
	glGenTextures(2, updated);
	glBindTexture(GL_TEXTURE_2D, updated[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, DISP_MAP_SIZE, DISP_MAP_SIZE);

	glBindTexture(GL_TEXTURE_2D, updated[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, DISP_MAP_SIZE, DISP_MAP_SIZE);

	glGenTextures(1, &tempdata);
	glBindTexture(GL_TEXTURE_2D, tempdata);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, DISP_MAP_SIZE, DISP_MAP_SIZE);

	// create displacement map
	glGenTextures(1, &displacement);
	glBindTexture(GL_TEXTURE_2D, displacement);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, DISP_MAP_SIZE, DISP_MAP_SIZE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// create gradient & folding map
	glGenTextures(1, &gradients);
	glBindTexture(GL_TEXTURE_2D, gradients);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, DISP_MAP_SIZE, DISP_MAP_SIZE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy / 2);

	glBindTexture(GL_TEXTURE_2D, 0);

	// create mesh and LOD levels (could use tess shader in the future)
	OpenGLVertexElement decl[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	numlods = Math::Log2OfPow2(MESH_SIZE);

	if (!GLCreateMesh((MESH_SIZE + 1) * (MESH_SIZE + 1), IndexCounts[numlods], GLMESH_32BIT, decl, &oceanmesh))
		return false;

	OpenGLAttributeRange* subsettable = nullptr;
	Math::Vector3* vdata = nullptr;
	uint32_t* idata = nullptr;
	GLuint numsubsets = 0;

	oceanmesh->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	oceanmesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		float tilesize = PATCH_SIZE / MESH_SIZE;

		// vertex data
		for (int z = 0; z <= MESH_SIZE; ++z) {
			for (int x = 0; x <= MESH_SIZE; ++x) {
				int index = z * (MESH_SIZE + 1) + x;

				vdata[index].x = (float)x;
				vdata[index].y = (float)z;
				vdata[index].z = 0.0f;
			}
		}

		// index data
		GenerateLODLevels(&subsettable, &numsubsets, idata);
	}
	oceanmesh->UnlockIndexBuffer();
	oceanmesh->UnlockVertexBuffer();

	oceanmesh->SetAttributeTable(subsettable, numsubsets);
	delete[] subsettable;

	// load shaders
	char defines[128];

	sprintf_s(defines, "#define DISP_MAP_SIZE	%d\n#define LOG2_DISP_MAP_SIZE	%d\n#define TILE_SIZE_X2	%.4f\n#define INV_TILE_SIZE	%.4f\n",
		DISP_MAP_SIZE,
		Math::Log2OfPow2(DISP_MAP_SIZE),
		PATCH_SIZE * 2.0f / DISP_MAP_SIZE,
		DISP_MAP_SIZE / PATCH_SIZE);

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/debugspectra.frag", &debugeffect, defines))
		return false;

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/updatespectrum.comp", &updatespectrum, defines))
		return false;

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/fourier_dft.comp", &fourier_dft, defines))
		return false;

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/fourier_fft.comp", &fourier_fft, defines))
		return false;

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/createdisplacement.comp", &createdisp, defines))
		return false;

	if (!GLCreateComputeProgramFromFile("../../Media/ShadersGL/creategradients.comp", &creategrad, defines))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/ocean.vert", 0, 0, 0, "../../Media/ShadersGL/ocean.frag", &oceaneffect, defines))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/ocean.vert", 0, 0, 0, "../../Media/ShadersGL/simplecolor.frag", &wireeffect, defines))
		return false;

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/sky.vert", 0, 0, 0, "../../Media/ShadersGL/sky.frag", &skyeffect))
		return false;

	screenquad = new OpenGLScreenQuad();

	// NOTE: can't query image bindings
	updatespectrum->SetInt("tilde_h0", 0);
	updatespectrum->SetInt("frequencies", 1);
	updatespectrum->SetInt("tilde_h", 2);
	updatespectrum->SetInt("tilde_D", 3);

	fourier_dft->SetInt("readbuff", 0);
	fourier_dft->SetInt("writebuff", 1);

	fourier_fft->SetInt("readbuff", 0);
	fourier_fft->SetInt("writebuff", 1);

	createdisp->SetInt("heightmap", 0);
	createdisp->SetInt("choppyfield", 1);
	createdisp->SetInt("displacement", 2);

	creategrad->SetInt("displacement", 0);
	creategrad->SetInt("gradients", 1);

	oceaneffect->SetInt("displacement", 0);
	oceaneffect->SetInt("perlin", 1);
	oceaneffect->SetInt("envmap", 2);
	oceaneffect->SetInt("gradients", 3);

	float white[] = { 1, 1, 1, 1 };

	wireeffect->SetInt("displacement", 0);
	wireeffect->SetInt("perlin", 1);
	wireeffect->SetVector("matColor", white);

	skyeffect->SetInt("sampler0", 0);
	debugeffect->SetInt("sampler0", 0);

	// other
	if (!GLCreateTextureFromFile("../../Media/Textures/perlin_noise.dds", false, &perlintex))
		return false;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxanisotropy / 2);

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/sky.qm", &skymesh))
		return false;

	if (!GLCreateCubeTextureFromDDS("../../Media/Textures/ocean_env.dds", true, &environment))
		return false;

	glGenVertexArrays(1, &debugvao);
	glBindVertexArray(debugvao);
	{
		// empty
	}
	glBindVertexArray(0);

	// init quadtree
	float ocean_extent = PATCH_SIZE * (1 << FURTHEST_COVER);
	float ocean_start[2] = { -0.5f * ocean_extent, -0.5f * ocean_extent };

	tree.Initialize(ocean_start, ocean_extent, (int)numlods, MESH_SIZE, PATCH_SIZE, MAX_COVERAGE, (float)(screenwidth * screenheight));

	// setup camera
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetClipPlanes(1.0f, 2000.0f);
	camera.SetSpeed(camera.GetSpeed() * 3.0f);

	camera.SetEyePosition(0, 15, 0);
	camera.SetOrientation(Math::DegreesToRadians(30), Math::DegreesToRadians(15), 0);

	// reproable VFC bug (see CHOOPY_SCALE_CORRECTION)
	//camera.SetEyePosition(39.043621f, 7.9374733f, -71.800049f);
	//camera.SetOrientation(1.7278752f, 1.5707960f, 0);

	// test position for article
	//camera.SetEyePosition(-0.068656817f, 3.6943157f, 11.333683f);
	//camera.SetOrientation(1.5446160f, 0.49741900f, 0);
	//camera.SetOrientation(-2.7881632, 0.20943955, 0);

	// setup debug camera
	debugcamera.SetFov(Math::DegreesToRadians(60));
	debugcamera.SetAspect((float)screenwidth / (float)screenheight);
	debugcamera.SetClipPlanes(1.0f, 2000.0f);
	debugcamera.SetSpeed(camera.GetSpeed() * 6.0f);

	debugcamera.SetEyePosition(0, 15, 0);
	debugcamera.SetOrientation(Math::DegreesToRadians(30), Math::DegreesToRadians(15), 0);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Use WASD and mouse to move around\n1-8: Change ocean color\n\nT - Toggle FFT/DFT\nR - Toggle debug camera\nH - Toggle help text",
		helptext, 512, 512);

	return true;
}

void GenerateLODLevels(OpenGLAttributeRange** subsettable, GLuint* numsubsets, uint32_t* idata)
{
#define CALC_INNER_INDEX(x, z) \
	((top + (z)) * (MESH_SIZE + 1) + left + (x))
// END

	assert(subsettable);
	assert(numsubsets);

	*numsubsets = (numlods - 2) * 3 * 3 * 3 * 3 * 2;
	*subsettable = new OpenGLAttributeRange[*numsubsets];

	int currsubset = 0;
	GLuint indexoffset = 0;
	GLuint numwritten = 0;
	OpenGLAttributeRange* subset = 0;

	//int numrestarts = 0;

	for (uint32_t level = 0; level < numlods - 2; ++level) {
		int levelsize = MESH_SIZE >> level;
		int mindegree = levelsize >> 3;

		for (int left_degree = levelsize; left_degree > mindegree; left_degree >>= 1) {
			for (int right_degree = levelsize; right_degree > mindegree; right_degree >>= 1) {
				for (int bottom_degree = levelsize; bottom_degree > mindegree; bottom_degree >>= 1) {
					for (int top_degree = levelsize; top_degree > mindegree; top_degree >>= 1) {
						int right	= ((right_degree == levelsize) ? levelsize : levelsize - 1);
						int left	= ((left_degree == levelsize) ? 0 : 1);
						int bottom	= ((bottom_degree == levelsize) ? levelsize : levelsize - 1);
						int top		= ((top_degree == levelsize) ? 0 : 1);

						// generate inner mesh (triangle strip)
						int width = right - left;
						int height = bottom - top;

						numwritten = 0;

						for (int z = 0; z < height; ++z) {
							if ((z & 1) == 1) {
								idata[numwritten++] = CALC_INNER_INDEX(0, z);
								idata[numwritten++] = CALC_INNER_INDEX(0, z + 1);

								for (int x = 0; x < width; ++x) {
									idata[numwritten++] = CALC_INNER_INDEX(x + 1, z);
									idata[numwritten++] = CALC_INNER_INDEX(x + 1, z + 1);
								}

								idata[numwritten++] = UINT32_MAX;
								//++numrestarts;
							} else {
								idata[numwritten++] = CALC_INNER_INDEX(width, z + 1);
								idata[numwritten++] = CALC_INNER_INDEX(width, z);

								for (int x = width - 1; x >= 0; --x) {
									idata[numwritten++] = CALC_INNER_INDEX(x, z + 1);
									idata[numwritten++] = CALC_INNER_INDEX(x, z);
								}

								idata[numwritten++] = UINT32_MAX;
								//++numrestarts;
							}
						}

						// add inner subset
						subset = ((*subsettable) + currsubset);

						subset->AttribId		= currsubset;
						subset->Enabled			= (numwritten > 0);
						subset->IndexCount		= numwritten;
						subset->IndexStart		= indexoffset;
						subset->PrimitiveType	= GL_TRIANGLE_STRIP;
						subset->VertexCount		= 0;
						subset->VertexStart		= 0;

						indexoffset += numwritten;
						idata += numwritten;

						++currsubset;

						// generate boundary mesh (triangle list)
						numwritten = GenerateBoundaryMesh(left_degree, top_degree, right_degree, bottom_degree, levelsize, idata);

						// add boundary subset
						subset = ((*subsettable) + currsubset);

						subset->AttribId		= currsubset;
						subset->Enabled			= (numwritten > 0);
						subset->IndexCount		= numwritten;
						subset->IndexStart		= indexoffset;
						subset->PrimitiveType	= GL_TRIANGLES;
						subset->VertexCount		= 0;
						subset->VertexStart		= 0;

						indexoffset += numwritten;
						idata += numwritten;

						++currsubset;
					}
				}
			}
		}
	}

	//OpenGLAttributeRange& lastsubset = (*subsettable)[currsubset - 1];
	//printf("Total indices: %lu (%lu restarts)\n", lastsubset.IndexStart + lastsubset.IndexCount, numrestarts);
}

GLuint GenerateBoundaryMesh(int deg_left, int deg_top, int deg_right, int deg_bottom, int levelsize, uint32_t* idata)
{
#define CALC_BOUNDARY_INDEX(x, z) \
	((z) * (MESH_SIZE + 1) + (x))
// END

	GLuint numwritten = 0;

	// top edge
	if (deg_top < levelsize) {
		int t_step = levelsize / deg_top;

		for (int i = 0; i < levelsize; i += t_step) {
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i, 0);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i + t_step / 2, 1);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i + t_step, 0);

			for (int j = 0; j < t_step / 2; ++j) {
				if (i == 0 && j == 0 && deg_left < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(i, 0);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j, 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j + 1, 1);
			}

			for (int j = t_step / 2; j < t_step; ++j) {
				if (i == levelsize - t_step && j == t_step - 1 && deg_right < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + t_step, 0);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j, 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j + 1, 1);
			}
		}
	}

	// left edge
	if (deg_left < levelsize) {
		int l_step = levelsize / deg_left;

		for (int i = 0; i < levelsize; i += l_step) {
			idata[numwritten++] = CALC_BOUNDARY_INDEX(0, i);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(0, i + l_step);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(1, i + l_step / 2);

			for (int j = 0; j < l_step / 2; ++j) {
				if (i == 0 && j == 0 && deg_top < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(0, i);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(1, i + j + 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(1, i + j);
			}

			for (int j = l_step / 2; j < l_step; ++j) {
				if (i == levelsize - l_step && j == l_step - 1 && deg_bottom < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(0, i + l_step);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(1, i + j + 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(1, i + j);
			}
		}
	}

	// right edge
	if (deg_right < levelsize) {
		int r_step = levelsize / deg_right;

		for (int i = 0; i < levelsize; i += r_step) {
			idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize, i);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize - 1, i + r_step / 2);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize, i + r_step);

			for (int j = 0; j < r_step / 2; ++j) {
				if (i == 0 && j == 0 && deg_top < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize, i);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize - 1, i + j);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize - 1, i + j + 1);
			}

			for (int j = r_step / 2; j < r_step; ++j) {
				if (i == levelsize - r_step && j == r_step - 1 && deg_bottom < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize, i + r_step);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize - 1, i + j);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(levelsize - 1, i + j + 1);
			}
		}
	}

	// bottom edge
	if (deg_bottom < levelsize) {
		int b_step = levelsize / deg_bottom;

		for (int i = 0; i < levelsize; i += b_step) {
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i, levelsize);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i + b_step, levelsize);
			idata[numwritten++] = CALC_BOUNDARY_INDEX(i + b_step / 2, levelsize - 1);

			for (int j = 0; j < b_step / 2; ++j) {
				if (i == 0 && j == 0 && deg_left < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(i, levelsize);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j + 1, levelsize - 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j, levelsize - 1);
			}

			for (int j = b_step / 2; j < b_step; ++j) {
				if (i == levelsize - b_step && j == b_step - 1 && deg_right < levelsize)
					continue;

				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + b_step, levelsize);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j + 1, levelsize - 1);
				idata[numwritten++] = CALC_BOUNDARY_INDEX(i + j, levelsize - 1);
			}
		}
	}

	return numwritten;
}

void UninitScene()
{
	delete oceanmesh;
	delete skymesh;
	delete skyeffect;
	delete oceaneffect;
	delete wireeffect;
	delete debugeffect;
	delete updatespectrum;
	delete fourier_dft;
	delete fourier_fft;
	delete createdisp;
	delete creategrad;
	delete screenquad;

	glDeleteVertexArrays(1, &debugvao);

	glDeleteTextures(1, &displacement);
	glDeleteTextures(1, &gradients);
	glDeleteTextures(1, &initial);
	glDeleteTextures(1, &frequencies);
	glDeleteTextures(2, updated);
	glDeleteTextures(1, &tempdata);
	glDeleteTextures(1, &environment);
	glDeleteTextures(1, &perlintex);
	glDeleteTextures(1, &helptext);

	OpenGLContentManager().Release();
}

void KeyDown(KeyCode key)
{
	if (use_debug)
		debugcamera.OnKeyDown(key);
	else
		camera.OnKeyDown(key);
}

void KeyUp(KeyCode key)
{
	if (use_debug)
		debugcamera.OnKeyUp(key);
	else
		camera.OnKeyUp(key);

	const int numcolors = ARRAY_SIZE(OceanColors);

	if (key >= KeyCode1 && key < KeyCode1 + numcolors) {
		currentcolor = key - KeyCode1;
	} else switch (key) {
	case KeyCodeT:
		use_fft = !use_fft;
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	case KeyCodeR: {
		use_debug = !use_debug;

		Math::Vector3 eye, ypr;

		camera.GetEyePosition(eye);
		camera.GetOrientation(ypr);

		debugcamera.SetEyePosition(eye[0], eye[1], eye[2]);
		debugcamera.SetOrientation(ypr[0], ypr[1], ypr[2]);
		} break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	if (use_debug)
		debugcamera.OnMouseMove(dx, dy);
	else
		camera.OnMouseMove(dx, dy);
}

void MouseDown(MouseButton button, int32_t x, int32_t y)
{
	if (use_debug)
		debugcamera.OnMouseDown(button);
	else
		camera.OnMouseDown(button);
}

void MouseUp(MouseButton button, int32_t x, int32_t y)
{
	if (use_debug)
		debugcamera.OnMouseUp(button);
	else
		camera.OnMouseUp(button);
}

void Update(float delta)
{
	if (use_debug)
		debugcamera.Update(delta);
	else
		camera.Update(delta);
}

void FourierTransform(GLuint spectrum)
{
	OpenGLEffect* effect = (use_fft ? fourier_fft : fourier_dft);

	// horizontal pass
	glBindImageTexture(0, spectrum, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(1, tempdata, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

	effect->Begin();
	{
		glDispatchCompute(DISP_MAP_SIZE, 1, 1);
	}
	effect->End();

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// vertical pass
	glBindImageTexture(0, tempdata, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(1, spectrum, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

	effect->Begin();
	{
		glDispatchCompute(DISP_MAP_SIZE, 1, 1);
	}
	effect->End();
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0.0f;

	Math::Matrix world, view, proj;
	Math::Matrix viewproj, debugviewproj;
	Math::Vector3 eye, debugeye;

	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screenwidth, screenheight);

	if (use_debug) {
		debugcamera.Animate(alpha);
		debugcamera.GetViewMatrix(view);
		debugcamera.GetProjectionMatrix(proj);
		debugcamera.GetEyePosition(debugeye);

		Math::MatrixMultiply(debugviewproj, view, proj);
	}

	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);

	// update spectra
	updatespectrum->SetFloat("time", time);

	glBindImageTexture(0, initial, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(1, frequencies, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);

	glBindImageTexture(2, updated[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
	glBindImageTexture(3, updated[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);

	updatespectrum->Begin();
	{
		glDispatchCompute(DISP_MAP_SIZE / 16, DISP_MAP_SIZE / 16, 1);
	}
	updatespectrum->End();

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// transform spectra to spatial/time domain
	FourierTransform(updated[0]);
	FourierTransform(updated[1]);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// calculate displacement map
	glBindImageTexture(0, updated[0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(1, updated[1], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32F);
	glBindImageTexture(2, displacement, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	createdisp->Begin();
	{
		glDispatchCompute(DISP_MAP_SIZE / 16, DISP_MAP_SIZE / 16, 1);
	}
	createdisp->End();

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// calculate normal & folding map
	glBindImageTexture(0, displacement, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, gradients, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	creategrad->Begin();
	{
		glDispatchCompute(DISP_MAP_SIZE / 16, DISP_MAP_SIZE / 16, 1);
	}
	creategrad->End();

	glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_2D, gradients);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// render sky
	glEnable(GL_FRAMEBUFFER_SRGB);

	Math::MatrixScaling(world, 5.0f, 5.0f, 5.0f);

	world._41 = eye.x;
	world._42 = eye.y;
	world._43 = eye.z;

	skyeffect->SetMatrix("matWorld", world);
	skyeffect->SetMatrix("matViewProj", viewproj);
	skyeffect->SetVector("eyePos", eye);

	if (!use_debug) {
		skyeffect->Begin();
		{
			glDepthMask(GL_FALSE);
			glBindTexture(GL_TEXTURE_CUBE_MAP, environment);

			skymesh->Draw();

			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			glDepthMask(GL_TRUE);
		}
		skyeffect->End();
	}

	// build quadtree
	tree.Rebuild(viewproj, proj, eye);

	// render ocean
	Math::Matrix	flipYZ			= { 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 };
	Math::Matrix	localtraf;
	Math::Vector4	uvparams		= { 0, 0, 0, 0 };
	Math::Vector4	perlinoffset	= { 0, 0, 0, 0 };
	Math::Vector2	w				= WIND_DIRECTION;
	int				pattern[4];
	OpenGLEffect*	effect			= (use_debug ? wireeffect : oceaneffect);
	GLuint			subset			= 0;

	uvparams.x = 1.0f / PATCH_SIZE;
	uvparams.y = 0.5f / DISP_MAP_SIZE;

	perlinoffset.x = -w.x * time * 0.06f;
	perlinoffset.y = -w.y * time * 0.06f;

	oceaneffect->SetMatrix("matViewProj", viewproj);
	oceaneffect->SetVector("perlinOffset", perlinoffset);
	oceaneffect->SetVector("eyePos", eye);
	oceaneffect->SetVector("oceanColor", OceanColors[currentcolor]);

	wireeffect->SetMatrix("matViewProj", debugviewproj);
	wireeffect->SetVector("perlinOffset", perlinoffset);
	wireeffect->SetVector("eyePos", debugeye);

	if (use_debug)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	effect->Begin();
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, displacement);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, perlintex);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, environment);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gradients);

		tree.Traverse([&](const TerrainQuadTree::Node& node) {
			float levelsize = (float)(MESH_SIZE >> node.lod);
			float scale = node.length / levelsize;

			Math::MatrixScaling(localtraf, scale, scale, 0);

			Math::MatrixTranslation(world, node.start[0], 0, node.start[1]);
			Math::MatrixMultiply(world, flipYZ, world);

			uvparams.z = node.start[0] / PATCH_SIZE;
			uvparams.w = node.start[1] / PATCH_SIZE;

			effect->SetMatrix("matLocal", localtraf);
			effect->SetMatrix("matWorld", world);
			effect->SetVector("uvParams", uvparams);
			effect->CommitChanges();

			tree.FindSubsetPattern(pattern, node);
			subset = CalcSubsetIndex(node.lod, pattern[0], pattern[1], pattern[2], pattern[3]);

			if (subset < oceanmesh->GetNumSubsets() - 1) {
				oceanmesh->DrawSubset(subset);
				oceanmesh->DrawSubset(subset + 1);
			}
		});

		glActiveTexture(GL_TEXTURE0);
	}
	effect->End();

	if (use_debug)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_FRAMEBUFFER_SRGB);
	time += elapsedtime;

	// TODO: draw spectra

	if (drawtext) {
		// render text
		glViewport(10, screenheight - 522, 512, 512);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, helptext);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();
	}

	// reset states
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// check errors
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		std::cout << "Error\n";

	app->Present();
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
	app->MouseMoveCallback = MouseMove;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;

	app->Run();
	delete app;

	return 0;
}
