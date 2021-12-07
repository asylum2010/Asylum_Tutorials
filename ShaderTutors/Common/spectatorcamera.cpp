
#include "spectatorcamera.h"

#define MOVEMENT_SPEED			1.4f	// m/s
#define ROTATIONAL_SPEED		0.75f	// rad/s
#define ROTATIONAL_INVINTERTIA	5.0f

SpectatorCamera::SpectatorCamera()
{
	aspect		= 4.0f / 3.0f;
	fov			= Math::DegreesToRadians(80);
	nearplane	= 0.1f;
	farplane	= 50.0f;
	speed		= MOVEMENT_SPEED;
	state		= 0;
	finished	= true;
	inertial	= true;

	anglecurve.Set(0, 0, 0);
	positioncurve.Set(0, 1.8f, 0);

	smoothedposition.x = positioncurve[0];
	smoothedposition.y = positioncurve[1];
	smoothedposition.z = positioncurve[2];

	Math::MatrixIdentity(view);
}

void SpectatorCamera::FitToBox(const Math::AABox& box)
{
	Math::Vector3 forward(-view._13, -view._23, -view._33);
	Math::Vector2 clip;

	Math::FitToBoxPerspective(clip, smoothedposition, forward, box);

	nearplane = clip.x;
	farplane = clip.y;
}

void SpectatorCamera::GetEyePosition(Math::Vector3& out)
{
	out = smoothedposition;
}

void SpectatorCamera::GetOrientation(Math::Vector3& out)
{
	out = smoothedangles;
}

void SpectatorCamera::GetViewMatrix(Math::Matrix& out)
{
	out = view;
}

void SpectatorCamera::GetViewVectors(Math::Vector3& forward, Math::Vector3& right, Math::Vector3& up)
{
	Math::Matrix yaw, pitch;

	Math::MatrixRotationAxis(yaw, anglecurve[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, anglecurve[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	// rotation matrix is right-handed
	forward = Math::Vector3(-view._13, -view._23, -view._33);
	right = Math::Vector3(view._11, view._21, view._31);
	up = Math::Vector3(view._12, view._22, view._32);
}

void SpectatorCamera::GetProjectionMatrix(Math::Matrix& out)
{
	Math::MatrixPerspectiveFovRH(out, fov, aspect, nearplane, farplane);
}

void SpectatorCamera::SetEyePosition(float x, float y, float z)
{
	smoothedposition = Math::Vector3(x, y, z);

	positioncurve.Set(x, y, z);
	finished = false;
}

void SpectatorCamera::SetOrientation(float yaw, float pitch, float roll)
{
	targetangles = Math::Vector3(yaw, pitch, roll);
	smoothedangles = Math::Vector3(yaw, pitch, roll);

	anglecurve.Set(yaw, pitch, roll);
	finished = false;
}

void SpectatorCamera::Update(float dt)
{
	Math::Vector3 forward, right, up;
	Math::Vector3 diff;
	Math::Vector2 movedir;

	// rotate
	targetangles[1] = Math::Clamp(targetangles[1], -Math::HALF_PI, Math::HALF_PI);

	diff[0] = (targetangles[0] - anglecurve[0]) * dt * ROTATIONAL_INVINTERTIA;
	diff[1] = (targetangles[1] - anglecurve[1]) * dt * ROTATIONAL_INVINTERTIA;
	diff[2] = 0;

	if (state != 0)
		finished = false;
	else if (!inertial || Math::Vec3Dot(diff, diff) < 1e-8f)
		finished = true;

	anglecurve.Advance(diff);
	
	// move
	diff = Math::Vector3(0, 0, 0);

	if (state & CameraStateMoving) {
		if (state & CameraStateLeft)
			movedir[0] = -1;

		if (state & CameraStateRight)
			movedir[0] = 1;

		if (state & CameraStateForward)
			movedir[1] = 1;

		if (state & CameraStateBackward)
			movedir[1] = -1;

		GetViewVectors(forward, right, up);

		Math::Vec3Scale(forward, forward, movedir[1]);
		Math::Vec3Scale(right, right, movedir[0]);
		Math::Vec3Add(diff, forward, right);

		Math::Vec3Normalize(diff, diff);
		Math::Vec3Scale(diff, diff, dt * speed);
	}

	positioncurve.Advance(diff);
}

void SpectatorCamera::Animate(float alpha)
{
	Math::Matrix yaw, pitch;

	if (inertial) {
		anglecurve.Smooth(smoothedangles, alpha);
		positioncurve.Smooth(smoothedposition, alpha);
	} else {
		smoothedangles = targetangles;
		smoothedposition = Math::Vector3(positioncurve[0], positioncurve[1], positioncurve[2]);
	}

	// recalculate view matrix
	Math::MatrixRotationAxis(yaw, smoothedangles[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, smoothedangles[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	view._41 = -(smoothedposition[0] * view._11 + smoothedposition[1] * view._21 + smoothedposition[2] * view._31);
	view._42 = -(smoothedposition[0] * view._12 + smoothedposition[1] * view._22 + smoothedposition[2] * view._32);
	view._43 = -(smoothedposition[0] * view._13 + smoothedposition[1] * view._23 + smoothedposition[2] * view._33);
}

void SpectatorCamera::OnKeyDown(KeyCode keycode)
{
	if (keycode == KeyCodeW)
		state |= CameraStateForward;

	if (keycode == KeyCodeS)
		state |= CameraStateBackward;

	if (keycode == KeyCodeD)
		state |= CameraStateRight;

	if (keycode == KeyCodeA)
		state |= CameraStateLeft;
}

void SpectatorCamera::OnKeyUp(KeyCode keycode)
{
	if (keycode == KeyCodeW)
		state &= (~CameraStateForward);

	if (keycode == KeyCodeS)
		state &= (~CameraStateBackward);

	if (keycode == KeyCodeD)
		state &= (~CameraStateRight);

	if (keycode == KeyCodeA)
		state &= (~CameraStateLeft);
}

void SpectatorCamera::OnMouseMove(int16_t dx, int16_t dy)
{
	if (state & CameraStateRotating) {
		float speedx = Math::DegreesToRadians((float)dx);
		float speedy = Math::DegreesToRadians((float)dy);

		targetangles[0] += speedx * ROTATIONAL_SPEED;
		targetangles[1] += speedy * ROTATIONAL_SPEED;
	}
}

void SpectatorCamera::OnMouseDown(MouseButton button)
{
	if (button == MouseButtonLeft)
		state |= CameraStateRotating;
}

void SpectatorCamera::OnMouseUp(MouseButton button)
{
	if (button == MouseButtonLeft)
		state &= (~CameraStateRotating);
}
