
#ifndef _3DMATH_H_
#define _3DMATH_H_

#include <cstdint>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <string>

// what: FUNC_PROTO\(([^\)]+)\)
// with: $1

#define ARRAY_SIZE(x)			(sizeof(x) / sizeof(x[0]))
#define MAKE_DWORD_PTR(a, b)	reinterpret_cast<void*>(((a << 16) & 0xffff0000ULL) | b)

namespace Math {

static const float PI = 3.141592f;
static const float TWO_PI = 6.283185f;
static const float HALF_PI = 1.570796f;
static const float ONE_OVER_SQRT_2 = 0.7071067f;
static const float ONE_OVER_SQRT_TWO_PI = 0.39894228f;

// --- Structures -------------------------------------------------------------

struct Complex
{
	float a;
	float b;

	Complex();
	Complex(float re, float im);

	Complex& operator +=(const Complex& other);

	inline Complex operator +(const Complex& other)	{ return Complex(a + other.a, b + other.b); }
	inline Complex operator -(const Complex& other)	{ return Complex(a - other.a, b - other.b); }
	inline Complex operator *(const Complex& other)	{ return Complex(a * other.a - b * other.b, b * other.a + a * other.b); }
};

struct Vector2
{
	float x, y;

	Vector2();
	Vector2(const Vector2& other);
	Vector2(float _x, float _y);
	Vector2(const float* values);

	Vector2 operator +(const Vector2& v) const;
	Vector2 operator -(const Vector2& v) const;
	Vector2 operator *(float s) const;

	Vector2& operator =(const Vector2& other);

	inline operator float*()					{ return &x; }
	inline operator const float*() const		{ return &x; }

	inline float& operator [](int index)		{ return *(&x + index); }
	inline float operator [](int index) const	{ return *(&x + index); }
};

struct Vector3
{
	float x, y, z;

	Vector3();
	Vector3(const Vector3& other);
	Vector3(float _x, float _y, float _z);
	Vector3(const float* values);

	Vector3 operator *(const Vector3& v) const;
	Vector3 operator +(const Vector3& v) const;
	Vector3 operator -(const Vector3& v) const;
	Vector3 operator *(float s) const;

	Vector3 operator -() const;

	Vector3& operator =(const Vector3& other);
	Vector3& operator +=(const Vector3& other);
	Vector3& operator *=(float s);

	inline operator float*()					{ return &x; }
	inline operator const float*() const		{ return &x; }

	inline float& operator [](int index)		{ return *(&x + index); }
	inline float operator [](int index) const	{ return *(&x + index); }
};

struct Vector4
{
	float x, y, z, w;

	Vector4();
	Vector4(const Vector4& other);
	Vector4(const Vector3& v, float w);
	Vector4(const Vector2& v, float z, float w);
	Vector4(float _x, float _y, float _z, float _w);
	Vector4(const float* values);

	Vector4 operator +(const Vector4& v) const;
	Vector4 operator -(const Vector4& v) const;
	Vector4 operator *(float s) const;
	Vector4 operator /(float s) const;

	Vector4& operator =(const Vector4& other);
	Vector4& operator /=(float s);

	inline operator float*()					{ return &x; }
	inline operator const float*() const		{ return &x; }

	inline float& operator [](int index)		{ return *(&x + index); }
	inline float operator [](int index) const	{ return *(&x + index); }
};

struct Quaternion
{
	float x, y, z, w;

	Quaternion();
	Quaternion(const Quaternion& other);
	Quaternion(float _x, float _y, float _z, float _w);

	Quaternion& operator =(const Quaternion& other);

	inline operator float*()					{ return &x; }
	inline operator const float*() const		{ return &x; }

	inline float& operator [](int index)		{ return *(&x + index); }
	inline float operator [](int index) const	{ return *(&x + index); }
};

struct Matrix
{
	// NOTE: row-major (multiply with it from the right)

	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;	// translation goes here

	Matrix();
	Matrix(const Matrix& other);
	Matrix(float v11, float v22, float v33, float v44);
	Matrix(
		float v11, float v12, float v13, float v14,
		float v21, float v22, float v23, float v24,
		float v31, float v32, float v33, float v34,
		float v41, float v42, float v43, float v44);
	Matrix(const float* values);

	Matrix& operator =(const Matrix& other);

	inline operator float*()						{ return &_11; }
	inline operator const float*() const			{ return &_11; }

	inline float* operator [](int row)				{ return (&_11 + 4 * row); }
	inline const float* operator [](int row) const	{ return (&_11 + 4 * row); }
};

// --- Template classes -------------------------------------------------------

template <typename T, int n>
class InterpolationArray
{
	static_assert(n > 0, "InterpolationArray: template parameter must be greater than zero");

private:
	T prev[n];
	T curr[n];

public:
	InterpolationArray& operator =(T t[n]);

	void Advance(T t[n]);
	void Set(T v1, ...);
	void Smooth(T out[n], T alpha);

	inline T operator [](int index) const	{ return curr[index]; }
};

template <typename T, int n>
InterpolationArray<T, n>& InterpolationArray<T, n>::operator =(T t[n])
{
	for (int i = 0; i < n; ++i)
		prev[i] = curr[i] = t[i];

	return *this;
}

template <typename T, int n>
void InterpolationArray<T, n>::Advance(T t[n])
{
	for (int i = 0; i < n; ++i) {
		prev[i] = curr[i];
		curr[i] += t[i];
	}
}

template <typename T, int n>
void InterpolationArray<T, n>::Set(T v1, ...)
{
	va_list args;
	va_start(args, v1);

	prev[0] = curr[0] = v1;

	if (std::is_same<T, float>::value) {
		// promoted to double
		for (int i = 1; i < n; ++i) {
			prev[i] = curr[i] = (float)va_arg(args, double);
		}
	} else {
		// ignore
		for (int i = 1; i < n; ++i) {
			prev[i] = curr[i] = va_arg(args, T);
		}
	}

	va_end(args);
}

template <typename T, int n>
void InterpolationArray<T, n>::Smooth(T out[n], T alpha)
{
	for (int i = 0; i < n; ++i)
		out[i] = prev[i] + alpha * (curr[i] - prev[i]);
}

// --- Classes ----------------------------------------------------------------

class Float16
{
private:
	uint16_t bits;

public:
	Float16();
	explicit Float16(float f);
	explicit Float16(uint16_t s);

	Float16& operator =(const Float16& other);
	Float16& operator =(float f);
	Float16& operator =(uint16_t s);
	operator float() const;

	inline operator uint16_t() const { return bits; }
};

static_assert(sizeof(Float16) == sizeof(uint16_t),  "Math::Float16 must be 2 bytes in size");

class Color
{
public:
	float r, g, b, a;

	Color();
	Color(float _r, float _g, float _b, float _a);
	Color(uint32_t argb32);

	Color operator *(float f);

	Color& operator =(const Color& other);
	Color& operator *=(const Color& other);

	static Color Lerp(const Color& from, const Color& to, float frac);
	static Color sRGBToLinear(uint32_t argb32);
	static Color sRGBToLinear(uint8_t red, uint8_t green, uint8_t blue);
	static Color sRGBToLinear(float red, float green, float blue, float alpha);

	static inline uint8_t ArgbA32(uint32_t c)	{ return ((uint8_t)((c >> 24) & 0xff)); }
	static inline uint8_t ArgbR32(uint32_t c)	{ return ((uint8_t)((c >> 16) & 0xff)); }
	static inline uint8_t ArgbG32(uint32_t c)	{ return ((uint8_t)((c >> 8) & 0xff)); }
	static inline uint8_t ArgbB32(uint32_t c)	{ return ((uint8_t)(c & 0xff)); }

	operator uint32_t() const;

	inline operator float*()					{ return &r; }
	inline operator const float*() const		{ return &r; }
};

class AABox
{
public:
	Vector3 Min;
	Vector3 Max;

	AABox();
	AABox(const AABox& other);
	AABox(const Vector3& size);
	AABox(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);

	AABox& operator =(const AABox& other);

	bool Intersects(const AABox& other) const;

	void Add(float x, float y, float z);
	void Add(const Vector3& p);
	void GetCenter(Vector3& out) const;
	void GetSize(Vector3& out) const;
	void GetHalfSize(Vector3& out) const;
	void GetPlanes(Vector4 out[6]) const;
	void Inset(float dx, float dy, float dz);
	void TransformAxisAligned(const Matrix& m);

	float Radius() const;
	float RayIntersect(const Vector3& start, const Vector3& dir) const;
	float Nearest(const Vector4& from) const;
	float Farthest(const Vector4& from) const;
};

// --- Math functions ---------------------------------------------------------

uint8_t FloatToByte(float f);
int32_t ISqrt(int32_t n);
uint32_t NextPow2(uint32_t x);
uint32_t Log2OfPow2(uint32_t x);
uint32_t ReverseBits32(uint32_t bits);
uint32_t Vec3ToUbyte4(const Math::Vector3& v);

float Vec2Dot(const Vector2& a, const Vector2& b);
float Vec2Length(const Vector2& v);
float Vec2Distance(const Vector2& a, const Vector2& b);

float Vec3Dot(const Vector3& a, const Vector3& b);
float Vec3Length(const Vector3& v);
float Vec3Distance(const Vector3& a, const Vector3& b);

float PlaneDotCoord(const Vector4& plane, const Vector3& p);

void Vec2Normalize(Vector2& out, const Vector2& v);
void Vec2Subtract(Vector2& out, const Vector2& a, const Vector2& b);

void Vec3Lerp(Vector3& out, const Vector3& a, const Vector3& b, float s);
void Vec3Add(Vector3& out, const Vector3& a, const Vector3& b);
void Vec3Mad(Vector3& out, const Vector3& a, const Vector3& b, float s);
void Vec3Normalize(Vector3& out, const Vector3& v);
void Vec3Scale(Vector3& out, const Vector3& v, float scale);
void Vec3Subtract(Vector3& out, const Vector3& a, const Vector3& b);
void Vec3Cross(Vector3& out, const Vector3& a, const Vector3& b);
void Vec3Rotate(Vector3& out, const Vector3& v, const Quaternion& q);
void Vec3Transform(Vector4& out, const Vector3& v, const Matrix& m);
void Vec3TransformTranspose(Vector4& out, const Matrix& m, const Vector3& v);
void Vec3TransformCoord(Vector3& out, const Vector3& v, const Matrix& m);
void Vec3TransformCoordTranspose(Vector3& out, const Matrix& m, const Vector3& v);
void Vec3TransformNormal(Vector3& out, const Vector3& v, const Matrix& m);
void Vec3TransformNormalTranspose(Vector3& out, const Matrix& m, const Vector3& v);

void Vec4Lerp(Vector4& out, const Vector4& a, const Vector4& b, float s);
void Vec4Add(Vector4& out, const Vector4& a, const Vector4& b);
void Vec4Subtract(Vector4& out, const Vector4& a, const Vector4& b);
void Vec4Scale(Vector4& out, const Vector4& v, float scale);
void Vec4Transform(Vector4& out, const Vector4& v, const Matrix& m);
void Vec4TransformTranspose(Vector4& out, const Matrix& m, const Vector4& v);

void PlaneFromNormalAndPoint(Vector4& out, const Vector3& n, const Vector3& p);
void PlaneFromTriangle(Vector4& out, const Vector3& a, const Vector3& b, const Vector3& c);
void PlaneNormalize(Vector4& out, const Vector4& p);
void PlaneTransform(Vector4& out, const Vector4& p, const Matrix& m);
void PlaneTransformTranspose(Vector4& out, const Matrix& m, const Vector4& p);

void QuaternionConjugate(Quaternion& out, const Quaternion& q);
void QuaternionIdentity(Quaternion& out);
void QuaternionMultiply(Quaternion& out, const Quaternion& a, const Quaternion& b);
void QuaternionNormalize(Quaternion& out, const Quaternion& q);
void QuaternionRotationAxis(Quaternion& out, float angle, float x, float y, float z);

void MatrixIdentity(Matrix& out);
void MatrixInverse(Matrix& out, const Matrix& m);
void MatrixLookAtLH(Matrix& out, const Vector3& eye, const Vector3& look, const Vector3& up);
void MatrixLookAtRH(Matrix& out, const Vector3& eye, const Vector3& look, const Vector3& up);
void MatrixMultiply(Matrix& out, const Matrix& a, const Matrix& b);
void MatrixPerspectiveFovLH(Matrix& out, float fovy, float aspect, float nearplane, float farplane);
void MatrixPerspectiveFovRH(Matrix& out, float fovy, float aspect, float nearplane, float farplane);
void MatrixOrthoOffCenterLH(Matrix& out, float left, float right, float bottom, float top, float nearplane, float farplane);
void MatrixOrthoOffCenterRH(Matrix& out, float left, float right, float bottom, float top, float nearplane, float farplane);
void MatrixReflect(Math::Matrix& out, const Math::Vector4& plane);
void MatrixRotationAxis(Matrix& out, float angle, float x, float y, float z);
void MatrixRotationQuaternion(Matrix& out, const Quaternion& q);
void MatrixScaling(Matrix& out, float x, float y, float z);
void MatrixTranslation(Matrix& out, float x, float y, float z);
void MatrixTranspose(Matrix& out, Matrix m);
void MatrixViewVector(Matrix& out, const Vector3& viewdir);

void FrustumPlanes(Math::Vector4 out[6], const Math::Matrix& viewproj);
void FitToBoxPerspective(Vector2& out, const Vector3& refpoint, const Vector3& forward, const AABox& box);
void FitToBoxOrtho(Matrix& outproj, Vector2& outclip, const Matrix& view, const AABox& box);
void GetOrthogonalVectors(Vector3& out1, Vector3& out2, const Vector3& v);

int FrustumIntersect(const Math::Vector4 frustum[6], const AABox& box);

// --- Utility functions ------------------------------------------------------

float F0FromEta(float eta);
float Gaussian(float x, float stddev);
float ImagePlaneArea(const Matrix& proj);

std::string& GetPath(std::string& out, const std::string& str);
std::string& GetFile(std::string& out, const std::string& str);
std::string& GetExtension(std::string& out, const std::string& str);
std::string& ToLower(std::string& out, const std::string& str);

void SaveToTGA(const std::string& file, uint32_t width, uint32_t height, void* pixels);

// --- Inline functions -------------------------------------------------------

template <typename T>
inline const T& Min(const T& a, const T& b) {
	return ((a < b) ? a : b);
}

template <typename T>
inline const T& Max(const T& a, const T& b) {
	return ((a > b) ? a : b);
}

template <typename T>
void Swap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

inline float DegreesToRadians(float value) {
	return value * (PI / 180.0f);
}

inline float RadiansToDegrees(float value) {
	return value * (180.0f / PI);
}

inline float Clamp(float value, float min, float max) {
	return Min(Max(min, value), max);
}

inline float RandomFloat() {
	return (rand() & 32767) / 32767.0f;
}

inline float Lerp(float a, float b, float s) {
	return (1.0f - s) * a + s * b;
}

inline float VerticalFov(float hfov, float w, float h) {
	return atan2f(h * tanf(hfov * 0.5f), w) * 2;
}

inline float HorizontalFov(float vfov, float w, float h) {
	return atan2f(w * tanf(vfov * 0.5f), h) * 2;
}

inline uint32_t FloatBitsToUint(float value) {
	return *((uint32_t*)&value);
}

inline float UintBitsToFloat(uint32_t value) {
	return *((float*)&value);
}

inline float EMToDIP(float emsize) {
	return (emsize / 96.0f) * 72.0f;
}

inline float DIPToEM(float points) {
	return (points / 72.0f) * 96.0f;
}

inline uint32_t As_Uint(float value) {
	return *((uint32_t*)&value);
}

}

// --- Operator overloads -----------------------------------------------------

Math::Vector2 operator *(float f, const Math::Vector2& v);
Math::Vector3 operator *(float f, const Math::Vector3& v);
Math::Vector4 operator *(float f, const Math::Vector4& v);

Math::Vector4 operator /(float f, const Math::Vector4& v);

Math::Color operator *(float f, const Math::Color& color);

#endif
