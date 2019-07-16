
#ifndef _FPSCAMERA_H_
#define _FPSCAMERA_H_

#include "3Dmath.h"
#include "application.h"

class PhysicsWorld;
class RigidBody;

class FPSCamera
{
	enum CameraState
	{
		CameraStateRotating = 1,
		CameraStateForward = 2,
		CameraStateBackward = 4,
		CameraStateLeft = 8,
		CameraStateRight = 16,
		CameraStateMoving = (CameraStateForward|CameraStateBackward|CameraStateLeft|CameraStateRight)
	};

	typedef Math::InterpolationArray<float, 3> CurveVector;

private:
	CurveVector		anglecurve;

	Math::Matrix	view;
	Math::Vector3	targetangles;
	Math::Vector3	smoothedangles;
	Math::Vector3	position;
	Math::Vector3	slopenormal;	// to be able to walk up stairs/slopes
	Math::Vector2	pitchlimits;

	PhysicsWorld*	physicsworld;
	RigidBody*		body;
	RigidBody*		slopebody;		// to be able to walk up stairs/slopes
	float			nearplane;
	float			farplane;
	float			fov;
	float			aspect;
	uint32_t		state;
	bool			isonground;

	void GetViewVectors(Math::Vector3& forward, Math::Vector3& right, Math::Vector3& up);

public:
	FPSCamera(PhysicsWorld* world);

	void FitToBox(const Math::AABox& box);

	void GetEyePosition(Math::Vector3& out);
	void GetViewMatrix(Math::Matrix& out);
	void GetProjectionMatrix(Math::Matrix& out);

	void SetEyePosition(float x, float y, float z);
	void SetOrientation(float yaw, float pitch, float roll);

	void Update(float dt);
	void Animate(float alpha);

	void OnKeyDown(KeyCode key);
	void OnKeyUp(KeyCode key);
	void OnMouseMove(int16_t dx, int16_t dy);
	void OnMouseDown(MouseButton button);
	void OnMouseUp(MouseButton button);

	inline void SetAspect(float value)					{ aspect = value; }
	inline void SetFov(float value)						{ fov = value; }
	inline void SetClipPlanes(float znear, float zfar)	{ nearplane = znear; farplane = zfar; }
	inline void SetPitchLimits(float low, float high)	{ pitchlimits = { low, high }; }
	//inline void SetSpeed(float value)					{ speed = value; }

	inline float GetAspect() const						{ return aspect; }
	inline float GetNearPlane() const					{ return nearplane; }
	inline float GetFarPlane() const					{ return farplane; }
	inline float GetFov() const							{ return fov; }
	//inline float GetSpeed() const						{ return speed; }
};

#endif
