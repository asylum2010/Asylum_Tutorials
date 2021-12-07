
#include "basiccamera.h"

#define ROTATIONAL_SPEED		0.75f	// rad/s
#define ROTATIONAL_INVINTERTIA	5.0f
#define POSITIONAL_INVINTERTIA	5.0f

BasicCamera::BasicCamera()
{
	distance	= 1.0f;
	nearplane	= 0.1f;
	farplane	= 50.0f;
	fov			= Math::PI / 3;
	aspect		= 4.0f / 3.0f;
	minzoom		= 0;
	maxzoom		= 30;
	minpitch	= -Math::HALF_PI;
	maxpitch	= Math::HALF_PI;
	finished	= true;
	inertial	= true;

	// NOTE: ignore XCode warning, the template handles promotion
	anglecurve.Set(0, 0, 0);
	pancurve.Set(0, 0, 0);
	zoomcurve.Set(0);
}

void BasicCamera::OrbitRight(float angle)
{
	targetangles[0] += angle * ROTATIONAL_SPEED;
	finished = false;
}

void BasicCamera::OrbitUp(float angle)
{
	targetangles[1] += angle * ROTATIONAL_SPEED;
	targetangles[1] = Math::Clamp(targetangles[1], minpitch, maxpitch);

	finished = false;
}

void BasicCamera::RollRight(float angle)
{
	targetangles[2] += angle * ROTATIONAL_SPEED;
	finished = false;
}

void BasicCamera::PanRight(float offset)
{
	Math::Matrix	view;
	Math::Matrix	yaw;
	Math::Matrix	pitch;
	Math::Vector3	right;

	Math::MatrixRotationAxis(yaw, smoothedangles[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, smoothedangles[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	right = Math::Vector3(view._11, view._21, view._31);
	Math::Vec3Mad(targetpan, targetpan, right, offset);

	finished = false;
}

void BasicCamera::PanUp(float offset)
{
	Math::Matrix	view;
	Math::Matrix	yaw;
	Math::Matrix	pitch;
	Math::Vector3	up;

	Math::MatrixRotationAxis(yaw, smoothedangles[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, smoothedangles[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	up = Math::Vector3(view._12, view._22, view._32);
	Math::Vec3Mad(targetpan, targetpan, up, offset);

	finished = false;
}

void BasicCamera::Zoom(float offset)
{
	targetzoom = Math::Clamp(distance - offset, minzoom, maxzoom);
	finished = false;
}

void BasicCamera::GetViewAndEye(Math::Matrix& view, Math::Vector3& eye) const
{
	Math::Matrix	yaw;
	Math::Matrix	pitch;
	Math::Vector3	forward;

	Math::MatrixRotationAxis(yaw, smoothedangles[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, smoothedangles[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	// rotation matrix is right handed
	forward = Math::Vector3(-view._13, -view._23, -view._33);

	Math::Vec3Scale(forward, forward, distance);
	Math::Vec3Subtract(eye, position, forward);
}

void BasicCamera::GetViewMatrix(Math::Matrix& out) const
{
	Math::Vector3 eye;
	GetViewAndEye(out, eye);

	out._41 = -(eye[0] * out._11 + eye[1] * out._21 + eye[2] * out._31);
	out._42 = -(eye[0] * out._12 + eye[1] * out._22 + eye[2] * out._32);
	out._43 = -(eye[0] * out._13 + eye[1] * out._23 + eye[2] * out._33);
}

void BasicCamera::GetProjectionMatrix(Math::Matrix& out) const
{
	return Math::MatrixPerspectiveFovRH(out, fov, aspect, nearplane, farplane);
}

void BasicCamera::GetPosition(Math::Vector3& out) const
{
	out = position;
}

void BasicCamera::GetEyePosition(Math::Vector3& out) const
{
	Math::Matrix view;
	GetViewAndEye(view, out);
}

void BasicCamera::GetOrientation(Math::Vector3& out) const
{
	out = smoothedangles;
}

void BasicCamera::Update(float dt)
{
	float diff1[3];
	float diff2[3];
	float diff3;

	// rotate
	targetangles[1] = Math::Clamp(targetangles[1], -Math::HALF_PI, Math::HALF_PI);

	diff1[0] = (targetangles[0] - anglecurve[0]) * dt * ROTATIONAL_INVINTERTIA;
	diff1[1] = (targetangles[1] - anglecurve[1]) * dt * ROTATIONAL_INVINTERTIA;
	diff1[2] = (targetangles[2] - anglecurve[2]) * dt * ROTATIONAL_INVINTERTIA;

	anglecurve.Advance(diff1);

	// pan
	diff2[0] = (targetpan[0] - pancurve[0]) * dt * POSITIONAL_INVINTERTIA;
	diff2[1] = (targetpan[1] - pancurve[1]) * dt * POSITIONAL_INVINTERTIA;
	diff2[2] = (targetpan[2] - pancurve[2]) * dt * POSITIONAL_INVINTERTIA;

	pancurve.Advance(diff2);

	// zoom
	diff3 = (targetzoom - zoomcurve[0]) * dt * POSITIONAL_INVINTERTIA;
	zoomcurve.Advance(&diff3);

	if (Math::Vec3Dot(diff1, diff1) < 1e-4f && Math::Vec3Dot(diff2, diff2) < 1e-4f && diff3 < 1e-2f)
		finished = true;
}

void BasicCamera::Animate(float alpha)
{
	if (inertial) {
		anglecurve.Smooth(smoothedangles, alpha);
		pancurve.Smooth(position, alpha);
		zoomcurve.Smooth(&distance, alpha);
	} else {
		smoothedangles = targetangles;
		position = targetpan;
		distance = targetzoom;
	}
}

void BasicCamera::SetOrientation(float yaw, float pitch, float roll)
{
	targetangles = Math::Vector3(yaw, pitch, roll);
	smoothedangles = Math::Vector3(yaw, pitch, roll);

	anglecurve.Set(yaw, pitch, roll);
	finished = true;
}

void BasicCamera::SetPosition(float x, float y, float z)
{
	targetpan = Math::Vector3(x, y, z);
	position = Math::Vector3(x, y, z);

	pancurve.Set(x, y, z);
	finished = true;
}

void BasicCamera::SetDistance(float value)
{
	targetzoom = value;
	distance = value;

	zoomcurve.Set(value);
	finished = true;
}
