
#include "fpscamera.h"
#include "physicsworld.h"

#define MOVEMENT_SPEED			1.4f	// m/s
#define ROTATIONAL_SPEED		0.75f	// rad/s
#define ROTATIONAL_INVINTERTIA	5.0f	// 1 / intertia of rotation
#define CAMERA_RADIUS			0.25f
#define EYE_HEIGHT				1.8f

FPSCamera::FPSCamera(PhysicsWorld* world)
{
	aspect			= 4.0f / 3.0f;
	fov				= Math::DegreesToRadians(80);
	nearplane		= 0.1f;
	farplane		= 50.0f;
	state			= 0;
	body			= nullptr;
	slopebody		= nullptr;
	physicsworld	= world;
	isonground		= false;
	pitchlimits		= { -Math::HALF_PI, Math::HALF_PI };

	anglecurve.Set(0, 0, 0);

	position = Math::Vector3(0, EYE_HEIGHT, 0);

	Math::MatrixIdentity(view);

	if (world != nullptr) {
		body = world->AddDynamicSphere(CAMERA_RADIUS, 80);
		body->SetPosition(0, 0.1f, 0);
	}
}

void FPSCamera::FitToBox(const Math::AABox& box)
{
	Math::Vector3 forward(-view._13, -view._23, -view._33);
	Math::Vector2 clip;

	Math::FitToBoxPerspective(clip, position, forward, box);

	nearplane = clip.x;
	farplane = clip.y;
}

void FPSCamera::GetEyePosition(Math::Vector3& out)
{
	out = position;
}

void FPSCamera::GetViewMatrix(Math::Matrix& out)
{
	out = view;
}

void FPSCamera::GetViewVectors(Math::Vector3& forward, Math::Vector3& right, Math::Vector3& up)
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

void FPSCamera::GetProjectionMatrix(Math::Matrix& out)
{
	Math::MatrixPerspectiveFovRH(out, fov, aspect, nearplane, farplane);
}

void FPSCamera::SetEyePosition(float x, float y, float z)
{
	position = Math::Vector3(x, y, z);

	if (body != nullptr)
		body->SetPosition(x, y - (EYE_HEIGHT - CAMERA_RADIUS), z);
}

void FPSCamera::SetOrientation(float yaw, float pitch, float roll)
{
	targetangles = Math::Vector3(yaw, pitch, roll);
	smoothedangles = Math::Vector3(yaw, pitch, roll);

	anglecurve.Set(yaw, pitch, roll);
}

void FPSCamera::Update(float dt)
{
	Math::Vector3 forward, right, up;
	Math::Vector3 diff;
	Math::Vector2 movedir;

	// rotate
	targetangles[1] = Math::Clamp(targetangles[1], pitchlimits.x, pitchlimits.y);

	diff[0] = (targetangles[0] - anglecurve[0]) * dt * ROTATIONAL_INVINTERTIA;
	diff[1] = (targetangles[1] - anglecurve[1]) * dt * ROTATIONAL_INVINTERTIA;
	diff[2] = 0;

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

		if (forward[1] > 0.98f)
			forward = -up;

		if (forward[1] < -0.98f)
			forward = up;

		forward[1] = right[1] = 0;

		Math::Vec3Scale(forward, forward, movedir[1]);
		Math::Vec3Scale(right, right, movedir[0]);
		Math::Vec3Add(diff, forward, right);

		Math::Vec3Normalize(diff, diff);
		Math::Vec3Scale(diff, diff, MOVEMENT_SPEED);
	}

	// update body (NOTE: don't even try it with physics)
	CollisionData	data;
	RigidBody*		groundbody = nullptr;
	Math::Vector4	groundplane;
	Math::Vector4	hitparams;
	Math::Vector3	hitpos;
	Math::Vector3	prevpos;
	Math::Vector3	vel;
	Math::Vector3	deltavel = { 0, 0, 0 };
	Math::Vector3	down = { 0, -1, 0 };
	bool			wasonground = isonground;

	prevpos = body->GetPosition();

	if (wasonground)
		body->SetVelocity(diff);
	
	body->Integrate(dt);

	if (slopebody != nullptr) {
		// we are on a stair/slope
		down = -slopenormal;
	}

	// look for ground first (hitparams.xyz = normal, w = ray parameter)
	groundbody = physicsworld->RayIntersect(hitparams, prevpos, down);
	isonground = false;

	if (slopebody != nullptr && groundbody != slopebody) {
		// end of stair/slope
		down = { 0, -1, 0 };
		slopebody = physicsworld->RayIntersect(hitparams, prevpos, down);;
		
		// verify that we really left the stair/slope
		if (groundbody == slopebody) {
			slopebody = nullptr;
		} else {
			// TODO: handle this case (not that easy...)
		}
	}

	if (groundbody != nullptr && hitparams[3] >= 0) { // && hitparams[1] > 0.64f
		Math::Vec3Mad(hitpos, prevpos, down, hitparams[3]);
		Math::PlaneFromNormalAndPoint(groundplane, (const Math::Vector3&)hitparams, hitpos);
		Math::Vec3Subtract(vel, body->GetPosition(), prevpos);

		hitparams[3] = (CAMERA_RADIUS - groundplane[3] - Math::Vec3Dot((const Math::Vector3&)hitparams, prevpos)) / Math::Vec3Dot((const Math::Vector3&)hitparams, vel);

		// NOTE: this '-0.5f' (tunneling tolerance) depends on the value of dt
		if (hitparams[3] > -0.5f && hitparams[3] < 1.0f) {
			// resolve position
			body->ResolvePenetration(hitparams[3] * dt);

			isonground = true;

			// resolve velocity and integrate
			float length = Math::Vec3Length(diff);
			float cosa = Math::Vec3Dot(diff, (const Math::Vector3&)hitparams);

			Math::Vec3Mad(diff, diff, (const Math::Vector3&)hitparams, -cosa);

			if (fabs(length) > 1e-5f)
				Math::Vec3Scale(diff, diff, length / Math::Vec3Length(diff));

			body->SetVelocity(diff);
			body->IntegratePosition(dt);
		}
	}

	vel = body->GetVelocity();

	// now test every other object
	physicsworld->DetectCollisions(data, body);

	for (size_t i = 0; i < data.contacts.size(); ++i) {
		const Contact& contact = data.contacts[i];

		if (contact.body2 == groundbody)
			continue;

		if (contact.depth > 0) {
			body->ResolvePenetration(contact);

			float rel_vel = Math::Vec3Dot(contact.normal, vel);
			float impulse = rel_vel;

			Math::Vec3Mad(deltavel, deltavel, contact.normal, -impulse);

			if (contact.normal.y > 0.5f) {
				// found a stair/slope we can walk on
				slopebody = contact.body2;
				slopenormal = contact.normal;
			}
		}
	}

	Math::Vec3Add(vel, vel, deltavel);
	body->SetVelocity(vel);
}

void FPSCamera::Animate(float alpha)
{
	Math::Matrix yaw, pitch;

	anglecurve.Smooth(smoothedangles, alpha);
	body->GetInterpolatedPosition(position, alpha);

	position[1] += (EYE_HEIGHT - CAMERA_RADIUS);

	// recalculate view matrix
	Math::MatrixRotationAxis(yaw, smoothedangles[0], 0, 1, 0);
	Math::MatrixRotationAxis(pitch, smoothedangles[1], 1, 0, 0);
	Math::MatrixMultiply(view, yaw, pitch);

	view._41 = -(position[0] * view._11 + position[1] * view._21 + position[2] * view._31);
	view._42 = -(position[0] * view._12 + position[1] * view._22 + position[2] * view._32);
	view._43 = -(position[0] * view._13 + position[1] * view._23 + position[2] * view._33);
}

void FPSCamera::OnKeyDown(KeyCode keycode)
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

void FPSCamera::OnKeyUp(KeyCode keycode)
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

void FPSCamera::OnMouseMove(int16_t dx, int16_t dy)
{
	if (state & CameraStateRotating) {
		float speedx = Math::DegreesToRadians((float)dx);
		float speedy = Math::DegreesToRadians((float)dy);

		targetangles[0] += speedx * ROTATIONAL_SPEED;
		targetangles[1] += speedy * ROTATIONAL_SPEED;
	}
}

void FPSCamera::OnMouseDown(MouseButton button)
{
	if (button == MouseButtonLeft)
		state |= CameraStateRotating;
}

void FPSCamera::OnMouseUp(MouseButton button)
{
	if (button == MouseButtonLeft)
		state &= (~CameraStateRotating);
}
