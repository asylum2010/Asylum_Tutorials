
#include "3Dmath.h"

namespace Math {

// --- Structures impl --------------------------------------------------------

Complex::Complex()
{
	a = b = 0;
}

Complex::Complex(float re, float im)
{
	a = re;
	b = im;
}

Complex& Complex::operator +=(const Complex& other)
{
	a += other.a;
	b += other.b;

	return *this;
}

Vector2::Vector2()
{
	x = y = 0;
}

Vector2::Vector2(const Vector2& other)
{
	x = other.x;
	y = other.y;
}

Vector2::Vector2(float _x, float _y)
{
	x = _x;
	y = _y;
}

Vector2::Vector2(const float* values)
{
	x = values[0];
	y = values[1];
}

Vector2 Vector2::operator +(const Vector2& v) const
{
	return Vector2(x + v.x, y + v.y);
}

Vector2 Vector2::operator -(const Vector2& v) const
{
	return Vector2(x - v.x, y - v.y);
}

Vector2 Vector2::operator *(float s) const
{
	return Vector2(x * s, y * s);
}

Vector2& Vector2::operator =(const Vector2& other)
{
	x = other.x;
	y = other.y;

	return *this;
}

Vector3::Vector3()
{
	x = y = z = 0;
}

Vector3::Vector3(const Vector3& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
}

Vector3::Vector3(float _x, float _y, float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

Vector3::Vector3(const float* values)
{
	x = values[0];
	y = values[1];
	z = values[2];
}

Vector3 Vector3::operator *(const Vector3& v) const
{
	return Vector3(x * v.x, y * v.y, z * v.z);
}

Vector3 Vector3::operator +(const Vector3& v) const
{
	return Vector3(x + v.x, y + v.y, z + v.z);
}

Vector3 Vector3::operator -(const Vector3& v) const
{
	return Vector3(x - v.x, y - v.y, z - v.z);
}

Vector3 Vector3::operator *(float s) const
{
	return Vector3(x * s, y * s, z * s);
}

Vector3 Vector3::operator -() const
{
	return Vector3(-x, -y, -z);
}

Vector3& Vector3::operator =(const Vector3& other)
{
	x = other.x;
	y = other.y;
	z = other.z;

	return *this;
}

Vector3& Vector3::operator +=(const Vector3& other)
{
	x += other.x;
	y += other.y;
	z += other.z;

	return *this;
}

Vector3& Vector3::operator *=(float s)
{
	x *= s;
	y *= s;
	z *= s;

	return *this;
}

Vector4::Vector4()
{
	x = y = z = w = 0;
}

Vector4::Vector4(const Vector4& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
}

Vector4::Vector4(const Vector3& v, float w)
{
	x = v.x;
	y = v.y;
	z = v.z;

	this->w = w;
}

Vector4::Vector4(const Vector2& v, float z, float w)
{
	x = v.x;
	y = v.y;

	this->z = z;
	this->w = w;
}

Vector4::Vector4(float _x, float _y, float _z, float _w)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}

Vector4::Vector4(const float* values)
{
	x = values[0];
	y = values[1];
	z = values[2];
	w = values[3];
}

Vector4 Vector4::operator +(const Vector4& v) const
{
	return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
}

Vector4 Vector4::operator -(const Vector4& v) const
{
	return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
}

Vector4 Vector4::operator *(float s) const
{
	return Vector4(x * s, y * s, z * s, w * s);
}

Vector4 Vector4::operator /(float s) const
{
	return Vector4(x / s, y / s, z / s, w / s);
}

Vector4& Vector4::operator =(const Vector4& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;

	return *this;
}

Vector4& Vector4::operator /=(float s)
{
	x /= s;
	y /= s;
	z /= s;
	w /= s;

	return *this;
}

Quaternion::Quaternion()
{
	x = y = z = 0;
	w = 1;
}

Quaternion::Quaternion(const Quaternion& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
}

Quaternion::Quaternion(float _x, float _y, float _z, float _w)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}

Quaternion& Quaternion::operator =(const Quaternion& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;

	return *this;
}

Matrix::Matrix()
{
	_11 = _12 = _13 = _14 = 0;
	_21 = _22 = _23 = _24 = 0;
	_31 = _32 = _33 = _34 = 0;
	_41 = _42 = _43 = _44 = 0;
}

Matrix::Matrix(const Matrix& other)
{
	operator =(other);
}

Matrix::Matrix(float v11, float v22, float v33, float v44)
{
	_12 = _13 = _14 = 0;
	_21 = _23 = _24 = 0;
	_31 = _32 = _34 = 0;
	_41 = _42 = _43 = 0;

	_11 = v11;
	_22 = v22;
	_33 = v33;
	_44 = v44;
}

Matrix::Matrix(
	float v11, float v12, float v13, float v14,
	float v21, float v22, float v23, float v24,
	float v31, float v32, float v33, float v34,
	float v41, float v42, float v43, float v44)
{
	_11 = v11;	_21 = v21;	_31 = v31;	_41 = v41;
	_12 = v12;	_22 = v22;	_32 = v32;	_42 = v42;
	_13 = v13;	_23 = v23;	_33 = v33;	_43 = v43;
	_14 = v14;	_24 = v24;	_34 = v34;	_44 = v44;
}

Matrix::Matrix(const float* values)
{
	_11 = values[0];	_21 = values[4];
	_12 = values[1];	_22 = values[5];
	_13 = values[2];	_23 = values[6];
	_14 = values[3];	_24 = values[7];

	_31 = values[8];	_41 = values[12];
	_32 = values[9];	_42 = values[13];
	_33 = values[10];	_43 = values[14];
	_34 = values[11];	_44 = values[15];
}

Matrix& Matrix::operator =(const Matrix& other)
{
	_11 = other._11;	_21 = other._21;
	_12 = other._12;	_22 = other._22;
	_13 = other._13;	_23 = other._23;
	_14 = other._14;	_24 = other._24;

	_31 = other._31;	_41 = other._41;
	_32 = other._32;	_42 = other._42;
	_33 = other._33;	_43 = other._43;
	_34 = other._34;	_44 = other._44;

	return *this;
}

// --- Float16 impl -----------------------------------------------------------

Float16::Float16()
{
	bits = 0;
}

Float16::Float16(float f)
{
	operator =(f);
}

Float16::Float16(uint16_t s)
{
	bits = s;
}

Float16& Float16::operator =(float f)
{
	uint32_t fp32		= *((uint32_t*)&f);
	uint32_t signbit	= (fp32 & 0x80000000) >> 16;
	uint32_t mant		= (fp32 & 0x007fffff);
	int exp				= ((fp32 & 0x7f800000) >> 23) - 112;

	if (exp <= 0)
		bits = 0;
	else if (exp > 30)
		bits = (uint16_t)(signbit|0x7bff);
	else
		bits = (uint16_t)(signbit|(exp << 10)|(mant >> 13));

	return *this;
}

Float16& Float16::operator =(uint16_t s)
{
	bits = s;
	return *this;
}

Float16& Float16::operator =(const Float16& other)
{
	bits = other.bits;
	return *this;
}

Float16::operator float() const
{
	uint32_t magic	= 126 << 23;
	uint32_t fp32	= (bits & 0x8000) << 16;
	uint32_t mant	= (bits & 0x000003ff);
	int exp			= (bits >> 10) & 0x0000001f;

	if (exp == 0) {
		fp32 = magic + mant;
		(*(float*)&fp32) -= (*(float*)&magic);
	} else {
		mant <<= 13;

		if (exp == 31)
			exp = 255;
		else
			exp += 127 - 15;

		fp32 |= (exp << 23);
		fp32 |= mant;
	}

	return *((float*)&fp32);
}

// --- Color impl -------------------------------------------------------------

Color::Color()
	: r(0), g(0), b(0), a(1)
{
}

Color::Color(float _r, float _g, float _b, float _a)
	: r(_r), g(_g), b(_b), a(_a)
{
}

Color::Color(uint32_t argb32)
{
	a = ArgbA32(argb32) / 255.0f;
	r = ArgbR32(argb32) / 255.0f;
	g = ArgbG32(argb32) / 255.0f;
	b = ArgbB32(argb32) / 255.0f;
}

Color Color::operator *(float f)
{
	return Color(r * f, g * f, b * f, a);
}

Color& Color::operator =(const Color& other)
{
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;

	return *this;
}

Color& Color::operator *=(const Color& other)
{
	r *= other.r;
	g *= other.g;
	b *= other.b;
	a *= other.a;

	return *this;
}

Color Color::Lerp(const Color& from, const Color& to, float frac)
{
	Color ret;

	ret.r = from.r * (1 - frac) + to.r * frac;
	ret.g = from.g * (1 - frac) + to.g * frac;
	ret.b = from.b * (1 - frac) + to.b * frac;
	ret.a = from.a * (1 - frac) + to.a * frac;

	return ret;
}

Color Color::sRGBToLinear(uint32_t argb32)
{
	Color ret = sRGBToLinear(ArgbR32(argb32), ArgbG32(argb32), ArgbB32(argb32));
	ret.a = ArgbA32(argb32) / 255.0f;

	return ret;
}

Color Color::sRGBToLinear(uint8_t red, uint8_t green, uint8_t blue)
{
	Color ret;

	float lo_r = (float)red / 3294.6f;
	float lo_g = (float)green / 3294.6f;
	float lo_b = (float)blue / 3294.6f;

	float hi_r = powf((red / 255.0f + 0.055f) / 1.055f, 2.4f);
	float hi_g = powf((green / 255.0f + 0.055f) / 1.055f, 2.4f);
	float hi_b = powf((blue / 255.0f + 0.055f) / 1.055f, 2.4f);

	ret.r = (red < 10 ? lo_r : hi_r);
	ret.g = (green < 10 ? lo_g : hi_g);
	ret.b = (blue < 10 ? lo_b : hi_b);
	ret.a = 1;

	return ret;
}

Color Color::sRGBToLinear(float red, float green, float blue, float alpha)
{
	Color ret;

	float lo_r = (float)red / 12.92f;
	float lo_g = (float)green / 12.92f;
	float lo_b = (float)blue / 12.92f;

	float hi_r = powf((red + 0.055f) / 1.055f, 2.4f);
	float hi_g = powf((green + 0.055f) / 1.055f, 2.4f);
	float hi_b = powf((blue + 0.055f) / 1.055f, 2.4f);

	ret.r = (red < 0.04f ? lo_r : hi_r);
	ret.g = (green < 0.04f ? lo_g : hi_g);
	ret.b = (blue < 0.04f ? lo_b : hi_b);
	ret.a = alpha;

	return ret;
}

Color::operator uint32_t() const
{
	uint8_t _a = (uint8_t)(a * 255);
	uint8_t _r = (uint8_t)(r * 255);
	uint8_t _g = (uint8_t)(g * 255);
	uint8_t _b = (uint8_t)(b * 255);

	return (_a << 24) | (_r << 16) | (_g << 8) | _b;
}

// --- AABox impl -------------------------------------------------------------

static void Assign_If_Less(float& a, float b)
{
	if (a < b)
		a = b;
};

static void Assign_If_Greater(float& a, float b)
{
	if (a > b)
		a = b;
};

static void CalculateDistanceIncr(float& value, const Vector3& p, const Vector4& from)
{
	float d = from.x * p.x + from.y * p.y + from.z * p.z + from.w;
	Assign_If_Greater(value, d);
}

static void CalculateDistanceDecr(float& value, const Vector3& p, const Vector4& from)
{
	float d = from.x * p.x + from.y * p.y + from.z * p.z + from.w;
	Assign_If_Less(value, d);
}

AABox::AABox()
{
	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;
}

AABox::AABox(const AABox& other)
{
	operator =(other);
}

AABox::AABox(const Vector3& size)
{
	Min = size * -0.5f;
	Max = size * 0.5f;
}

AABox::AABox(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
{
	Min = Vector3(xmin, ymin, zmin);
	Max = Vector3(xmax, ymax, zmax);
}

AABox& AABox::operator =(const AABox& other)
{
	if (&other != this) {
		Min[0] = other.Min[0];
		Min[1] = other.Min[1];
		Min[2] = other.Min[2];

		Max[0] = other.Max[0];
		Max[1] = other.Max[1];
		Max[2] = other.Max[2];
	}

	return *this;
}

bool AABox::Intersects(const AABox& other) const
{
	if (Max[0] < other.Min[0] || Min[0] > other.Max[0])
		return false;

	if (Max[1] < other.Min[1] || Min[1] > other.Max[1])
		return false;

	if (Max[2] < other.Min[2] || Min[2] > other.Max[2])
		return false;

	return true;
}

void AABox::Add(float x, float y, float z)
{
	Assign_If_Less(Max[0], x);
	Assign_If_Less(Max[1], y);
	Assign_If_Less(Max[2], z);

	Assign_If_Greater(Min[0], x);
	Assign_If_Greater(Min[1], y);
	Assign_If_Greater(Min[2], z);
}

void AABox::Add(const Vector3& p)
{
	Assign_If_Less(Max[0], p[0]);
	Assign_If_Less(Max[1], p[1]);
	Assign_If_Less(Max[2], p[2]);

	Assign_If_Greater(Min[0], p[0]);
	Assign_If_Greater(Min[1], p[1]);
	Assign_If_Greater(Min[2], p[2]);
}

void AABox::GetCenter(Vector3& out) const
{
	out[0] = (Min[0] + Max[0]) * 0.5f;
	out[1] = (Min[1] + Max[1]) * 0.5f;
	out[2] = (Min[2] + Max[2]) * 0.5f;
}

void AABox::GetSize(Vector3& out) const
{
	out[0] = Max[0] - Min[0];
	out[1] = Max[1] - Min[1];
	out[2] = Max[2] - Min[2];
}

void AABox::GetHalfSize(Vector3& out) const
{
	out[0] = (Max[0] - Min[0]) * 0.5f;
	out[1] = (Max[1] - Min[1]) * 0.5f;
	out[2] = (Max[2] - Min[2]) * 0.5f;
}

void AABox::Inset(float dx, float dy, float dz)
{
	Min[0] += dx;
	Min[1] += dy;
	Min[2] += dz;

	Max[0] -= dx;
	Max[1] -= dy;
	Max[2] -= dz;
}

void AABox::TransformAxisAligned(const Matrix& m)
{
	Vector3 vertices[8] = {
		{ Min[0], Min[1], Min[2] },
		{ Min[0], Min[1], Max[2] },
		{ Min[0], Max[1], Min[2] },
		{ Min[0], Max[1], Max[2] },
		{ Max[0], Min[1], Min[2] },
		{ Max[0], Min[1], Max[2] },
		{ Max[0], Max[1], Min[2] },
		{ Max[0], Max[1], Max[2] }
	};
	
	for (int i = 0; i < 8; ++i)
		Vec3TransformCoord(vertices[i], vertices[i], m);

	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
		Add(vertices[i]);
}

void AABox::GetPlanes(Vector4 out[6]) const
{
	PlaneFromNormalAndPoint(out[0], { 1, 0, 0 }, { Min[0], Min[1], Min[2] });	// left
	PlaneFromNormalAndPoint(out[1], { -1, 0, 0 }, { Max[0], Min[1], Min[2] });	// right
	PlaneFromNormalAndPoint(out[2], { 0, 1, 0 }, { Min[0], Min[1], Min[2] });	// bottom
	PlaneFromNormalAndPoint(out[3], { 0, -1, 0 }, { Min[0], Max[1], Min[2] });	// top
	PlaneFromNormalAndPoint(out[4], { 0, 0, -1 }, { Min[0], Min[1], Max[2] });	// front
	PlaneFromNormalAndPoint(out[5], { 0, 0, 1 }, { Min[0], Min[1], Min[2] });	// back
}

float AABox::Radius() const
{
	return Vec3Distance(Min, Max) * 0.5f;
}

float AABox::RayIntersect(const Vector3& start, const Vector3& dir) const
{
	Vector3 m1, m2;
	float t1, t2, t3, t4, t5, t6;

	Vec3Subtract(m1, Min, start);
	Vec3Subtract(m2, Max, start);

	if (dir[0] == 0) {
		t1 = (m1[0] >= 0 ? FLT_MAX : -FLT_MAX);
		t2 = (m2[0] >= 0 ? FLT_MAX : -FLT_MAX);
	} else {
		t1 = m1[0] / dir[0];
		t2 = m2[0] / dir[0];
	}

	if (dir[1] == 0) {
		t3 = (m1[1] >= 0 ? FLT_MAX : -FLT_MAX);
		t4 = (m2[1] >= 0 ? FLT_MAX : -FLT_MAX);
	} else {
		t3 = m1[1] / dir[1];
		t4 = m2[1] / dir[1];
	}

	if (dir[2] == 0) {
		t5 = (m1[2] >= 0 ? FLT_MAX : -FLT_MAX);
		t6 = (m2[2] >= 0 ? FLT_MAX : -FLT_MAX);
	} else {
		t5 = m1[2] / dir[2];
		t6 = m2[2] / dir[2];
	}

	float tmin = Math::Max(Math::Max(Math::Min(t1, t2), Math::Min(t3, t4)), Math::Min(t5, t6));
	float tmax = Math::Min(Math::Min(Math::Max(t1, t2), Math::Max(t3, t4)), Math::Max(t5, t6));

	if (tmax < 0 || tmin > tmax)
		return FLT_MAX;

	return tmin;
}

float AABox::Nearest(const Vector4& from) const
{
	float dist = FLT_MAX;

	CalculateDistanceIncr(dist, { Min[0], Min[1], Min[2] }, from);
	CalculateDistanceIncr(dist, { Min[0], Min[1], Max[2] }, from);
	CalculateDistanceIncr(dist, { Min[0], Max[1], Min[2] }, from);
	CalculateDistanceIncr(dist, { Min[0], Max[1], Max[2] }, from);
	CalculateDistanceIncr(dist, { Max[0], Min[1], Min[2] }, from);
	CalculateDistanceIncr(dist, { Max[0], Min[1], Max[2] }, from);
	CalculateDistanceIncr(dist, { Max[0], Max[1], Min[2] }, from);
	CalculateDistanceIncr(dist, { Max[0], Max[1], Max[2] }, from);

	return dist;
}

float AABox::Farthest(const Vector4& from) const
{
	float dist = -FLT_MAX;

	CalculateDistanceDecr(dist, { Min[0], Min[1], Min[2] }, from);
	CalculateDistanceDecr(dist, { Min[0], Min[1], Max[2] }, from);
	CalculateDistanceDecr(dist, { Min[0], Max[1], Min[2] }, from);
	CalculateDistanceDecr(dist, { Min[0], Max[1], Max[2] }, from);
	CalculateDistanceDecr(dist, { Max[0], Min[1], Min[2] }, from);
	CalculateDistanceDecr(dist, { Max[0], Min[1], Max[2] }, from);
	CalculateDistanceDecr(dist, { Max[0], Max[1], Min[2] }, from);
	CalculateDistanceDecr(dist, { Max[0], Max[1], Max[2] }, from);

	return dist;
}

// --- Math functions impl ----------------------------------------------------

uint8_t FloatToByte(float f)
{
	int32_t i = (int32_t)f;
	
	if (i < 0)
		return 0;
	else if (i > 255)
		return 255;
	
	return (uint8_t)i;
}

int32_t ISqrt(int32_t n)
{
	int32_t b = 0;

	while (n >= 0) {
		n = n - b;
		b = b + 1;
		n = n - b;
	}

	return b - 1;
}

uint32_t NextPow2(uint32_t x)
{
	--x;

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return ++x;
}

uint32_t Log2OfPow2(uint32_t x)
{
	uint32_t ret = 0;

	while (x >>= 1)
		++ret;

	return ret;
}

uint32_t ReverseBits32(uint32_t bits)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);

	return bits;
}

uint32_t Vec3ToUbyte4(const Math::Vector3& v)
{
	uint32_t ret = 0;
	uint8_t* bytes = (uint8_t*)(&ret);
	
	bytes[0] = FloatToByte((v[0] + 1.0f) * (255.0f / 2.0f) + 0.5f);
	bytes[1] = FloatToByte((v[1] + 1.0f) * (255.0f / 2.0f) + 0.5f);
	bytes[2] = FloatToByte((v[2] + 1.0f) * (255.0f / 2.0f) + 0.5f);
	
	return ret;
}

float Vec2Dot(const Vector2& a, const Vector2& b)
{
	return (a.x * b.x + a.y * b.y);
}

float Vec2Length(const Vector2& v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

float Vec2Distance(const Vector2& a, const Vector2& b)
{
	return Vec2Length(a - b);
}

void Vec2Normalize(Vector2& out, const Vector2& v)
{
	float il = 1.0f / sqrtf(v.x * v.x + v.y * v.y);

	out[0] = v[0] * il;
	out[1] = v[1] * il;
}

void Vec2Subtract(Vector2& out, const Vector2& a, const Vector2& b)
{
	out.x = a.x - b.x;
	out.y = a.y - b.y;
}

float Vec3Dot(const Vector3& a, const Vector3& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

float Vec3Length(const Vector3& v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Vec3Distance(const Vector3& a, const Vector3& b)
{
	return Vec3Length(a - b);
}

float PlaneDotCoord(const Vector4& plane, const Vector3& p)
{
	return (plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w);
}

void Vec3Lerp(Vector3& out, const Vector3& a, const Vector3& b, float s)
{
	float invs = 1.0f - s;
	
	out[0] = a[0] * invs + b[0] * s;
	out[1] = a[1] * invs + b[1] * s;
	out[2] = a[2] * invs + b[2] * s;
}

void Vec3Add(Vector3& out, const Vector3& a, const Vector3& b)
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

void Vec3Mad(Vector3& out, const Vector3& a, const Vector3& b, float s)
{
	out[0] = a[0] + b[0] * s;
	out[1] = a[1] + b[1] * s;
	out[2] = a[2] + b[2] * s;
}

void Vec3Normalize(Vector3& out, const Vector3& v)
{
	float il = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

	out[0] = v[0] * il;
	out[1] = v[1] * il;
	out[2] = v[2] * il;
}

void Vec3Scale(Vector3& out, const Vector3& v, float scale)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

void Vec3Subtract(Vector3& out, const Vector3& a, const Vector3& b)
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

void Vec3Cross(Vector3& out, const Vector3& a, const Vector3& b)
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void Vec3Rotate(Vector3& out, const Vector3& v, const Quaternion& q)
{
	Quaternion cq;
	Quaternion p = { v[0], v[1], v[2], 0 };

	QuaternionConjugate(cq, q);

	QuaternionMultiply(p, p, cq);
	QuaternionMultiply(p, q, p);

	out[0] = p[0];
	out[1] = p[1];
	out[2] = p[2];
}

void Vec3Transform(Vector4& out, const Vector3& v, const Matrix& m)
{
	Vector4 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._21 + v[2] * m._31 + m._41;
	tmp[1] = v[0] * m._12 + v[1] * m._22 + v[2] * m._32 + m._42;
	tmp[2] = v[0] * m._13 + v[1] * m._23 + v[2] * m._33 + m._43;
	tmp[3] = v[0] * m._14 + v[1] * m._24 + v[2] * m._34 + m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void Vec3TransformTranspose(Vector4& out, const Matrix& m, const Vector3& v)
{
	Vector4 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._12 + v[2] * m._13 + m._14;
	tmp[1] = v[0] * m._21 + v[1] * m._22 + v[2] * m._23 + m._24;
	tmp[2] = v[0] * m._31 + v[1] * m._32 + v[2] * m._33 + m._34;
	tmp[3] = v[0] * m._41 + v[1] * m._42 + v[2] * m._43 + m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void Vec3TransformCoord(Vector3& out, const Vector3& v, const Matrix& m)
{
	Vector4 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._21 + v[2] * m._31 + m._41;
	tmp[1] = v[0] * m._12 + v[1] * m._22 + v[2] * m._32 + m._42;
	tmp[2] = v[0] * m._13 + v[1] * m._23 + v[2] * m._33 + m._43;
	tmp[3] = v[0] * m._14 + v[1] * m._24 + v[2] * m._34 + m._44;

	out[0] = tmp[0] / tmp[3];
	out[1] = tmp[1] / tmp[3];
	out[2] = tmp[2] / tmp[3];
}

void Vec3TransformCoordTranspose(Vector3& out, const Matrix& m, const Vector3& v)
{
	Vector4 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._12 + v[2] * m._13 + m._14;
	tmp[1] = v[0] * m._21 + v[1] * m._22 + v[2] * m._23 + m._24;
	tmp[2] = v[0] * m._31 + v[1] * m._32 + v[2] * m._33 + m._34;
	tmp[3] = v[0] * m._41 + v[1] * m._42 + v[2] * m._43 + m._44;

	out[0] = tmp[0] / tmp[3];
	out[1] = tmp[1] / tmp[3];
	out[2] = tmp[2] / tmp[3];
}

void Vec3TransformNormal(Vector3& out, const Vector3& v, const Matrix& m)
{
	// NOTE: expects inverse transpose
	Vector3 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._21 + v[2] * m._31;
	tmp[1] = v[0] * m._12 + v[1] * m._22 + v[2] * m._32;
	tmp[2] = v[0] * m._13 + v[1] * m._23 + v[2] * m._33;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
}

void Vec3TransformNormalTranspose(Vector3& out, const Matrix& m, const Vector3& v)
{
	// NOTE: expects inverse
	Vector3 tmp;

	tmp[0] = v[0] * m._11 + v[1] * m._12 + v[2] * m._13;
	tmp[1] = v[0] * m._21 + v[1] * m._22 + v[2] * m._23;
	tmp[2] = v[0] * m._31 + v[1] * m._32 + v[2] * m._33;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
}

void Vec4Lerp(Vector4& out, const Vector4& a, const Vector4& b, float s)
{
	float invs = 1.0f - s;
	
	out[0] = a[0] * invs + b[0] * s;
	out[1] = a[1] * invs + b[1] * s;
	out[2] = a[2] * invs + b[2] * s;
	out[3] = a[3] * invs + b[3] * s;
}

void Vec4Add(Vector4& out, const Vector4& a, const Vector4& b)
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
	out[3] = a[3] + b[3];
}

void Vec4Subtract(Vector4& out, const Vector4& a, const Vector4& b)
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
	out[3] = a[3] - b[3];
}

void Vec4Scale(Vector4& out, const Vector4& v, float scale)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
	out[3] = v[3] * scale;
}

void Vec4Transform(Vector4& out, const Vector4& v, const Matrix& m)
{
	float tmp[4];

	tmp[0] = v[0] * m._11 + v[1] * m._21 + v[2] * m._31 + v[3] * m._41;
	tmp[1] = v[0] * m._12 + v[1] * m._22 + v[2] * m._32 + v[3] * m._42;
	tmp[2] = v[0] * m._13 + v[1] * m._23 + v[2] * m._33 + v[3] * m._43;
	tmp[3] = v[0] * m._14 + v[1] * m._24 + v[2] * m._34 + v[3] * m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void Vec4TransformTranspose(Vector4& out, const Matrix& m, const Vector4& v)
{
	float tmp[4];

	tmp[0] = v[0] * m._11 + v[1] * m._12 + v[2] * m._13 + v[3] * m._14;
	tmp[1] = v[0] * m._21 + v[1] * m._22 + v[2] * m._23 + v[3] * m._24;
	tmp[2] = v[0] * m._31 + v[1] * m._32 + v[2] * m._33 + v[3] * m._34;
	tmp[3] = v[0] * m._41 + v[1] * m._42 + v[2] * m._43 + v[3] * m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void PlaneFromNormalAndPoint(Vector4& out, const Vector3& n, const Vector3& p)
{
	out.x = n.x;
	out.y = n.y;
	out.z = n.z;
	out.w = -Vec3Dot(p, n);
}

void PlaneFromTriangle(Vector4& out, const Vector3& a, const Vector3& b, const Vector3& c)
{
	Vector3 bma = b - a;
	Vector3 cma = c - a;

	Vec3Cross((Math::Vector3&)out, bma, cma);
	out.w = -Math::Vec3Dot((Math::Vector3&)out, a);

	PlaneNormalize(out, out);
}

void PlaneNormalize(Vector4& out, const Vector4& p)
{
	float il = 1.0f / Vec3Length((const Vector3&)p);

	out[0] = p[0] * il;
	out[1] = p[1] * il;
	out[2] = p[2] * il;
	out[3] = p[3] * il;
}

void PlaneTransform(Vector4& out, const Vector4& p, const Matrix& m)
{
	// NOTE: m is the inverse transpose of the original matrix
	float tmp[4];

	tmp[0] = p[0] * m._11 + p[1] * m._21 + p[2] * m._31 + p[3] * m._41;
	tmp[1] = p[0] * m._12 + p[1] * m._22 + p[2] * m._32 + p[3] * m._42;
	tmp[2] = p[0] * m._13 + p[1] * m._23 + p[2] * m._33 + p[3] * m._43;
	tmp[3] = p[0] * m._14 + p[1] * m._24 + p[2] * m._34 + p[3] * m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void PlaneTransformTranspose(Vector4& out, const Matrix& m, const Vector4& p)
{
	// NOTE: m is the inverse of the original matrix
	float tmp[4];

	tmp[0] = p[0] * m._11 + p[1] * m._12 + p[2] * m._13 + p[3] * m._14;
	tmp[1] = p[0] * m._21 + p[1] * m._22 + p[2] * m._23 + p[3] * m._24;
	tmp[2] = p[0] * m._31 + p[1] * m._32 + p[2] * m._33 + p[3] * m._34;
	tmp[3] = p[0] * m._41 + p[1] * m._42 + p[2] * m._43 + p[3] * m._44;

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void QuaternionConjugate(Quaternion& out, const Quaternion& q)
{
	out[0] = -q[0];
	out[1] = -q[1];
	out[2] = -q[2];
	out[3] = q[3];
}

void QuaternionIdentity(Quaternion& out)
{
	out[0] = out[1] = out[2] = 0;
	out[3] = 1;
}

void QuaternionMultiply(Quaternion& out, const Quaternion& a, const Quaternion& b)
{
	Quaternion tmp;

	tmp[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	tmp[1] = a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2];
	tmp[2] = a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0];
	tmp[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void QuaternionNormalize(Quaternion& out, const Quaternion& q)
{
	float il = 1.0f / sqrtf(q[3] * q[3] + q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);

	out[3] = q[3] * il;
	out[0] = q[0] * il;
	out[1] = q[1] * il;
	out[2] = q[2] * il;
}

void QuaternionRotationAxis(Quaternion& out, float angle, float x, float y, float z)
{
	float l = sqrtf(x * x + y * y + z * z);
	float ha = angle * 0.5f;
	float sa = sinf(ha);

	out[0] = (x / l) * sa;
	out[1] = (y / l) * sa;
	out[2] = (z / l) * sa;
	out[3] = cosf(ha);
}

void MatrixIdentity(Matrix& out)
{
	out._12 = out._13 = out._14 = 0;
	out._21 = out._23 = out._24 = 0;
	out._31 = out._32 = out._34 = 0;
	out._41 = out._42 = out._43 = 0;

	out._11 = out._22 = out._33 = out._44 = 1;
}

void MatrixInverse(Matrix& out, const Matrix& m)
{
	float s[6] = {
		m._11 * m._22 - m._12 * m._21,
		m._11 * m._23 - m._13 * m._21,
		m._11 * m._24 - m._14 * m._21,
		m._12 * m._23 - m._13 * m._22,
		m._12 * m._24 - m._14 * m._22,
		m._13 * m._24 - m._14 * m._23
	};

	float c[6] = {
		m._31 * m._42 - m._32 * m._41,
		m._31 * m._43 - m._33 * m._41,
		m._31 * m._44 - m._34 * m._41,
		m._32 * m._43 - m._33 * m._42,
		m._32 * m._44 - m._34 * m._42,
		m._33 * m._44 - m._34 * m._43
	};

	float det = (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

#ifdef _DEBUG
	if (fabs(det) < 1e-9f)
		throw 1;
#endif

	float r = 1.0f / det;

	out._11 = r * (m._22 * c[5] - m._23 * c[4] + m._24 * c[3]);
	out._12 = r * (m._13 * c[4] - m._12 * c[5] - m._14 * c[3]);
	out._13 = r * (m._42 * s[5] - m._43 * s[4] + m._44 * s[3]);
	out._14 = r * (m._33 * s[4] - m._32 * s[5] - m._34 * s[3]);

	out._21 = r * (m._23 * c[2] - m._21 * c[5] - m._24 * c[1]);
	out._22 = r * (m._11 * c[5] - m._13 * c[2] + m._14 * c[1]);
	out._23 = r * (m._43 * s[2] - m._41 * s[5] - m._44 * s[1]);
	out._24 = r * (m._31 * s[5] - m._33 * s[2] + m._34 * s[1]);

	out._31 = r * (m._21 * c[4] - m._22 * c[2] + m._24 * c[0]);
	out._32 = r * (m._12 * c[2] - m._11 * c[4] - m._14 * c[0]);
	out._33  = r * (m._41 * s[4] - m._42 * s[2] + m._44 * s[0]);
	out._34  = r * (m._32 * s[2] - m._31 * s[4] - m._34 * s[0]);

	out._41 = r * (m._22 * c[1] - m._21 * c[3] - m._23 * c[0]);
	out._42 = r * (m._11 * c[3] - m._12 * c[1] + m._13 * c[0]);
	out._43 = r * (m._42 * s[1] - m._41 * s[3] - m._43 * s[0]);
	out._44 = r * (m._31 * s[3] - m._32 * s[1] + m._33 * s[0]);
}

void MatrixLookAtLH(Matrix& out, const Vector3& eye, const Vector3& look, const Vector3& up)
{
	Vector3 x, y, z;

	Vec3Subtract(z, look, eye);
	Vec3Normalize(z, z);

	Vec3Cross(x, up, z);
	Vec3Normalize(x, x);

	Vec3Cross(y, z, x);
	Vec3Normalize(y, y);

	out._11 = x[0];		out._12 = y[0];		out._13 = z[0];		out._14 = 0.0f;
	out._21 = x[1];		out._22 = y[1];		out._23 = z[1];		out._24 = 0.0f;
	out._31 = x[2];		out._32 = y[2];		out._33 = z[2];		out._34 = 0.0f;

	// sign depends on input data
	out._41 = -Vec3Dot(x, eye);
	out._42 = -Vec3Dot(y, eye);
	out._43 = -Vec3Dot(z, eye);
	out._44 = 1.0f;
}

void MatrixLookAtRH(Matrix& out, const Vector3& eye, const Vector3& look, const Vector3& up)
{
	Vector3 x, y, z;

	Vec3Subtract(z, look, eye);
	Vec3Normalize(z, z);

	Vec3Cross(x, z, up);
	Vec3Normalize(x, x);

	Vec3Cross(y, x, z);
	Vec3Normalize(y, y);

	out._11 = x[0];		out._12 = y[0];		out._13 = -z[0];		out._14 = 0.0f;
	out._21 = x[1];		out._22 = y[1];		out._23 = -z[1];		out._24 = 0.0f;
	out._31 = x[2];		out._32 = y[2];		out._33 = -z[2];		out._34 = 0.0f;

	// sign depends on input data
	out._41 = -Vec3Dot(x, eye);
	out._42 = -Vec3Dot(y, eye);
	out._43 = Vec3Dot(z, eye);
	out._44 = 1.0f;
}

void MatrixMultiply(Matrix& out, const Matrix& a, const Matrix& b)
{
	Matrix tmp;

	tmp._11 = a._11 * b._11 + a._12 * b._21 + a._13 * b._31 + a._14 * b._41;
	tmp._12 = a._11 * b._12 + a._12 * b._22 + a._13 * b._32 + a._14 * b._42;
	tmp._13 = a._11 * b._13 + a._12 * b._23 + a._13 * b._33 + a._14 * b._43;
	tmp._14 = a._11 * b._14 + a._12 * b._24 + a._13 * b._34 + a._14 * b._44;

	tmp._21 = a._21 * b._11 + a._22 * b._21 + a._23 * b._31 + a._24 * b._41;
	tmp._22 = a._21 * b._12 + a._22 * b._22 + a._23 * b._32 + a._24 * b._42;
	tmp._23 = a._21 * b._13 + a._22 * b._23 + a._23 * b._33 + a._24 * b._43;
	tmp._24 = a._21 * b._14 + a._22 * b._24 + a._23 * b._34 + a._24 * b._44;

	tmp._31 = a._31 * b._11 + a._32 * b._21 + a._33 * b._31 + a._34 * b._41;
	tmp._32 = a._31 * b._12 + a._32 * b._22 + a._33 * b._32 + a._34 * b._42;
	tmp._33 = a._31 * b._13 + a._32 * b._23 + a._33 * b._33 + a._34 * b._43;
	tmp._34 = a._31 * b._14 + a._32 * b._24 + a._33 * b._34 + a._34 * b._44;

	tmp._41 = a._41 * b._11 + a._42 * b._21 + a._43 * b._31 + a._44 * b._41;
	tmp._42 = a._41 * b._12 + a._42 * b._22 + a._43 * b._32 + a._44 * b._42;
	tmp._43 = a._41 * b._13 + a._42 * b._23 + a._43 * b._33 + a._44 * b._43;
	tmp._44 = a._41 * b._14 + a._42 * b._24 + a._43 * b._34 + a._44 * b._44;

	out = tmp;
}

void MatrixPerspectiveFovLH(Matrix& out, float fovy, float aspect, float nearplane, float farplane)
{
#ifdef VULKAN
	out._22 = -1.0f / tanf(fovy / 2);
	out._11 = out._22 / -aspect;
#else
	out._22 = 1.0f / tanf(fovy / 2);
	out._11 = out._22 / aspect;
#endif

	out._12 = out._13 = out._14 = 0;
	out._21 = out._23 = out._24 = 0;
	out._31 = out._32 = 0;
	out._41 = out._42 = out._43 = 0;

	out._34 = 1.0f;

#if defined(VULKAN) || defined(METAL) || defined(DIRECT3D9) || defined(DIRECT3D10)
	// [0, 1]
	out._33 = farplane / (nearplane - farplane);
	out._43 = -nearplane * out._33;
#else
	// [-1, 1]
	out._33 = (farplane + nearplane) / (nearplane - farplane);
	out._43 = -2 * farplane * nearplane / (nearplane - farplane);
#endif
}

void MatrixPerspectiveFovRH(Matrix& out, float fovy, float aspect, float nearplane, float farplane)
{
#ifdef VULKAN
	out._22 = -1.0f / tanf(fovy / 2);
	out._11 = out._22 / -aspect;
#else
	out._22 = 1.0f / tanf(fovy / 2);
	out._11 = out._22 / aspect;
#endif

	out._12 = out._13 = out._14 = 0;
	out._21 = out._23 = out._24 = 0;
	out._31 = out._32 = 0;
	out._41 = out._42 = out._43 = 0;

	out._34 = -1.0f;

#if defined(VULKAN) || defined(METAL) || defined(DIRECT3D9) || defined(DIRECT3D10)
	// [0, 1]
	out._33 = farplane / (nearplane - farplane);
	out._43 = nearplane * out._33;
#else
	// [-1, 1]
	out._33 = (farplane + nearplane) / (nearplane - farplane);
	out._43 = 2 * farplane * nearplane / (nearplane - farplane);
#endif
}

void MatrixOrthoOffCenterLH(Matrix& out, float left, float right, float bottom, float top, float nearplane, float farplane)
{
	out._11 = 2.0f / (right - left);
	out._12 = out._13 = out._14 = 0;
	
	out._22 = 2.0f / (top - bottom);
	out._21 = out._23 = out._24 = 0;

#ifdef OPENGL
	// [-1, 1]
	out._33 = 2.0f / (farplane - nearplane);
	out._43 = -(farplane + nearplane) / (farplane - nearplane);

	out._41 = -(right + left) / (right - left);
	out._42 = -(top + bottom) / (top - bottom);
#else
	// [0, 1]
	out._33 = 1.0f / (farplane - nearplane);
	out._43 = -nearplane * out._33;

	out._41 = (right + left) / (left - right);
	out._42 = (top + bottom) / (bottom - top);
#endif

	out._31 = out._32 = out._34 = 0;
	out._44 = 1;
}

void MatrixOrthoOffCenterRH(Matrix& out, float left, float right, float bottom, float top, float nearplane, float farplane)
{
	out._11 = 2.0f / (right - left);
	out._12 = out._13 = out._14 = 0;
	
	out._22 = 2.0f / (top - bottom);
	out._21 = out._23 = out._24 = 0;

#ifdef OPENGL
	// [-1, 1]
	out._33 = -2.0f / (farplane - nearplane);
	out._43 = -(farplane + nearplane) / (farplane - nearplane);

	out._41 = -(right + left) / (right - left);
	out._42 = -(top + bottom) / (top - bottom);
#else
	// [0, 1]
	out._33 = 1.0f / (nearplane - farplane);
	out._43 = nearplane * out._33;

	out._41 = (right + left) / (left - right);
	out._42 = (top + bottom) / (bottom - top);
#endif

	out._31 = out._32 = out._34 = 0;
	out._44 = 1;
}

void MatrixReflect(Math::Matrix& out, const Math::Vector4& plane)
{
	Math::Vector4 p;
	PlaneNormalize(p, plane);

	out._11 = -2 * p[0] * p[0] + 1;
	out._12 = -2 * p[1] * p[0];
	out._13 = -2 * p[2] * p[0];

	out._21 = -2 * p[0] * p[1];
	out._22 = -2 * p[1] * p[1] + 1;
	out._23 = -2 * p[2] * p[1];

	out._31 = -2 * p[0] * p[2];
	out._32 = -2 * p[1] * p[2];
	out._33 = -2 * p[2] * p[2] + 1;

	out._41 = -2 * p[0] * p[3];
	out._42 = -2 * p[1] * p[3];
	out._43 = -2 * p[2] * p[3];

	out._14 = out._24 = out._34 = 0;
	out._44 = 1;
}

void MatrixRotationAxis(Matrix& out, float angle, float x, float y, float z)
{
	Vector3 u = { x, y, z };

	float cosa = cosf(angle);
	float sina = sinf(angle);

	Vec3Normalize(u, u);

	out._11 = cosa + u[0] * u[0] * (1.0f - cosa);
	out._21 = u[0] * u[1] * (1.0f - cosa) - u[2] * sina;
	out._31 = u[0] * u[2] * (1.0f - cosa) + u[1] * sina;
	out._41 = 0;

	out._12 = u[1] * u[0] * (1.0f - cosa) + u[2] * sina;
	out._22 = cosa + u[1] * u[1] * (1.0f - cosa);
	out._32 = u[1] * u[2] * (1.0f - cosa) - u[0] * sina;
	out._42 = 0;

	out._13 = u[2] * u[0] * (1.0f - cosa) - u[1] * sina;
	out._23 = u[2] * u[1] * (1.0f - cosa) + u[0] * sina;
	out._33 = cosa + u[2] * u[2] * (1.0f - cosa);
	out._43 = 0;

	out._14 = out._24 = out._34 = 0;
	out._44 = 1;
}

void MatrixRotationQuaternion(Matrix& out, const Quaternion& q)
{
	out._11 = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
	out._12 = 2.0f * (q[0] * q[1] + q[2] * q[3]);
	out._13 = 2.0f * (q[0] * q[2] - q[1] * q[3]);

	out._21 = 2.0f * (q[0] * q[1] - q[2] * q[3]);
	out._22 = 1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]);
	out._23 = 2.0f * (q[1] * q[2] + q[0] * q[3]);

	out._31 = 2.0f * (q[0] * q[2] + q[1] * q[3]);
	out._32 = 2.0f * (q[1] * q[2] - q[0] * q[3]);
	out._33 = 1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]);

	out._14 = out._24 = out._34 = 0;
	out._41 = out._42 = out._43 = 0;
	out._44 = 1;
}

void MatrixScaling(Matrix& out, float x, float y, float z)
{
	MatrixIdentity(out);

	out._11 = x;
	out._22 = y;
	out._33 = z;
}

void MatrixTranslation(Matrix& out, float x, float y, float z)
{
	MatrixIdentity(out);

	out._41 = x;
	out._42 = y;
	out._43 = z;
}

void MatrixTranspose(Matrix& out, Matrix m)
{
	out._11 = m._11;
	out._12 = m._21;
	out._13 = m._31;
	out._14 = m._41;
	
	out._21 = m._12;
	out._22 = m._22;
	out._23 = m._32;
	out._24 = m._42;
	
	out._31 = m._13;
	out._32 = m._23;
	out._33 = m._33;
	out._34 = m._43;
	
	out._41 = m._14;
	out._42 = m._24;
	out._43 = m._34;
	out._44 = m._44;
}

void MatrixViewVector(Matrix& out, const Vector3& viewdir)
{
	Vector3 x, z;
	Vector3 y = { 0, 1, 0 };

	Vec3Normalize(z, viewdir);

	if (fabs(z.y) > 0.999f) {
		y = { 1, 0, 0 };
	}

	Vec3Cross(x, y, z);
	Vec3Cross(y, z, x);

	out._11 = x[0];	out._12 = y[0];	out._13 = z[0];
	out._21 = x[1];	out._22 = y[1];	out._23 = z[1];
	out._31 = x[2];	out._32 = y[2];	out._33 = z[2];

	out._14 = out._24 = out._34 = out._41 = out._42 = out._43 = 0;
	out._44 = 1;
}

void FrustumPlanes(Math::Vector4 out[6], const Math::Matrix& viewproj)
{
	out[0] = { viewproj._11 + viewproj._14, viewproj._21 + viewproj._24, viewproj._31 + viewproj._34, viewproj._41 + viewproj._44 };	// left
	out[1] = { viewproj._14 - viewproj._11, viewproj._24 - viewproj._21, viewproj._34 - viewproj._31, viewproj._44 - viewproj._41 };	// right
	out[2] = { viewproj._14 - viewproj._12, viewproj._24 - viewproj._22, viewproj._34 - viewproj._32, viewproj._44 - viewproj._42 };	// top
	out[3] = { viewproj._12 + viewproj._14, viewproj._22 + viewproj._24, viewproj._32 + viewproj._34, viewproj._42 + viewproj._44 };	// bottom
	out[4] = { viewproj._13, viewproj._23, viewproj._33, viewproj._43 };																// near
	out[5] = { viewproj._14 - viewproj._13, viewproj._24 - viewproj._23, viewproj._34 - viewproj._33, viewproj._44 - viewproj._43 };	// far

	PlaneNormalize(out[0], out[0]);
	PlaneNormalize(out[1], out[1]);
	PlaneNormalize(out[2], out[2]);
	PlaneNormalize(out[3], out[3]);
	PlaneNormalize(out[4], out[4]);
	PlaneNormalize(out[5], out[5]);
}

void FitToBoxPerspective(Vector2& outclip, const Vector3& refpoint, const Vector3& forward, const AABox& box)
{
	// NOTE: for area lights you have to determine the projection matrix
	Vector4 refplane;

	refplane[0] = forward[0];
	refplane[1] = forward[1];
	refplane[2] = forward[2];
	refplane[3] = -Vec3Dot(forward, refpoint);

	PlaneNormalize(refplane, refplane);

	outclip.x = box.Nearest(refplane) - 2e-3f;	// 2mm
	outclip.y = box.Farthest(refplane) + 2e-3f;	// 2mm

	outclip.x = Max<float>(outclip.x, 0.1f);
	outclip.y = Max<float>(outclip.y, 0.2f);
}

void FitToBoxOrtho(Matrix& outproj, Vector2& outclip, const Matrix& view, const AABox& box)
{
	Vector4 pright(1, 0, 0, 0);
	Vector4 pbottom(0, 1, 0, 0);
	Vector4 pnear(0, 0, -1, 0);

	// transform planes to world space
	Vec4TransformTranspose(pright, view, pright);
	Vec4TransformTranspose(pbottom, view, pbottom);
	Vec4TransformTranspose(pnear, view, pnear);

	float left		= box.Nearest(pright) - pright.w;
	float right		= box.Farthest(pright) - pright.w;
	float bottom	= box.Nearest(pbottom) - pbottom.w;
	float top		= box.Farthest(pbottom) - pbottom.w;

	outclip[0] = box.Nearest(pnear) - 2e-3f;	// 2mm
	outclip[1] = box.Farthest(pnear) + 2e-3f;	// 2mm

	// NOTE: near/far are distances: [-near, -far] -> depth
	MatrixOrthoOffCenterRH(outproj, left, right, bottom, top, outclip[0], outclip[1]);
}

void GetOrthogonalVectors(Vector3& out1, Vector3& out2, const Vector3& v)
{
	// select dominant direction
	int bestcoord = 0;

	if (fabs(v[1]) > fabs(v[bestcoord]))
		bestcoord = 1;

	if (fabs(v[2]) > fabs(v[bestcoord]))
		bestcoord = 2;

	// ignore handedness
	int other1 = (bestcoord + 1) % 3;
	int other2 = (bestcoord + 2) % 3;

	out1[bestcoord] = v[other1];
	out1[other1] = -v[bestcoord];
	out1[other2] = v[other2];

	out2[bestcoord] = v[other2];
	out2[other1] = v[other1];
	out2[other2] = -v[bestcoord];
}

int FrustumIntersect(const Math::Vector4 frustum[6], const AABox& box)
{
	Math::Vector3 center, halfsize;
	float dist, maxdist;
	int result = 2; // inside

	box.GetCenter(center);
	box.GetHalfSize(halfsize);
	
	// NOTE: fast version, might give false negatives
	for (int j = 0; j < 6; ++j) {
		const Math::Vector4& plane = frustum[j];

		dist = PlaneDotCoord(plane, center);
		maxdist = fabs(plane[0] * halfsize[0]) + fabs(plane[1] * halfsize[1]) + fabs(plane[2] * halfsize[2]);

		if (dist < -maxdist)
			return 0;	// outside
		else if (fabs(dist) < maxdist)
			result = 1;	// intersect
	}

	return result;
}

// --- Utility functions impl -------------------------------------------------

float F0FromEta(float eta)
{
	float etam1 = eta - 1.0f;
	float etap1 = eta + 1.0f;

	return (etam1 * etam1) / (etap1 * etap1);
}

float Gaussian(float x, float stddev)
{
	return (ONE_OVER_SQRT_TWO_PI / stddev) * expf(-((x * x) / (2.0f * stddev * stddev)));
}

float ImagePlaneArea(const Matrix& proj)
{
	Matrix projinv;
	Vector4 c1(-1, -1, -1.0, 1.0);
	Vector4 c2(1, 1, -1.0, 1.0);

	MatrixInverse(projinv, proj);

	Vec4Transform(c1, c1, projinv);
	Vec4Transform(c2, c2, projinv);

	c1 /= c1.w;
	c2 /= c2.w;

	// want the result at z = 1
	c1 /= c1.z;
	c2 /= c2.z;

	return std::abs(c2.x - c1.x) * std::abs(c2.y - c1.y);
}

std::string& GetPath(std::string& out, const std::string& str)
{
	size_t pos = str.find_last_of("\\/");

	if (pos != std::string::npos) {
#ifdef _WIN32
		out = str.substr(0, pos) + '\\';
#else
		out = str.substr(0, pos) + '/';
#endif
	} else {
		out = "";
	}

	return out;
}

std::string& GetFile(std::string& out, const std::string& str)
{
	size_t pos = str.find_last_of("\\/");

	if (pos != std::string::npos)
		out = str.substr(pos + 1);
	else
		out = str;

	return out;
}

std::string& GetExtension(std::string& out, const std::string& str)
{
	size_t pos = str.find_last_of(".");

	if (pos == std::string::npos) {
		out = "";
		return out;
	}

	out = str.substr(pos + 1, str.length());
	return ToLower(out, out);
}

std::string& ToLower(std::string& out, const std::string& str)
{
	out.resize(str.size());

	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] >= 'A' && str[i] <= 'Z')
			out[i] = str[i] + 32;
		else
			out[i] = str[i];
	}

	return out;
}

void SaveToTGA(const std::string& file, uint32_t width, uint32_t height, void* pixels)
{
	FILE* outfile = nullptr;

#ifdef _WIN32
	fopen_s(&outfile, file.c_str(), "wb");
#else
	outfile = fopen(file.c_str(), "wb");
#endif
	
	if (!outfile)
		return;

	uint8_t header[18];
	memset(header, 0, 18);

	header[2] = 2;

	*((uint16_t*)(&header[12])) = (uint16_t)width;
	*((uint16_t*)(&header[14])) = (uint16_t)height;

	header[16] = 4 * 8;
		
	fwrite(header, 1, 18, outfile);
	fwrite(pixels, 1, width * height * 4, outfile);
		
	fclose(outfile);
}

}

// --- Operators impl ---------------------------------------------------------

Math::Vector2 operator *(float f, const Math::Vector2& v)
{
	return Math::Vector2(f * v.x, f * v.y);
}

Math::Vector3 operator *(float f, const Math::Vector3& v)
{
	return Math::Vector3(f * v.x, f * v.y, f * v.z);
}

Math::Vector4 operator *(float f, const Math::Vector4& v)
{
	return Math::Vector4(f * v.x, f * v.y, f * v.z, f * v.w);
}

Math::Vector4 operator /(float f, const Math::Vector4& v)
{
	return Math::Vector4(f / v.x, f / v.y, f / v.z, f / v.w);
}

Math::Color operator *(float f, const Math::Color& color)
{
	return Math::Color(f * color.r, f * color.g, f * color.b, color.a);
}
