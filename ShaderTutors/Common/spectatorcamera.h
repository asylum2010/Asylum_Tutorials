
#ifndef _SPECTATORCAMERA_H_
#define _SPECTATORCAMERA_H_

#include "3Dmath.h"
#include "application.h"

class SpectatorCamera
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
	CurveVector		positioncurve;

	Math::Matrix	view;
	Math::Vector3	targetangles;
	Math::Vector3	smoothedangles;
	Math::Vector3	smoothedposition;

	uint32_t		state;
	float			nearplane;
	float			farplane;
	float			fov;
	float			aspect;
	float			speed;
	bool			finished;
	bool			inertial;

	void GetViewVectors(Math::Vector3& forward, Math::Vector3& right, Math::Vector3& up);

public:
	SpectatorCamera();

	void FitToBox(const Math::AABox& box);

	void GetEyePosition(Math::Vector3& out);
	void GetOrientation(Math::Vector3& out);
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
	inline void SetSpeed(float value)					{ speed = value; }
	inline void SetIntertia(bool enable)				{ inertial = enable; }

	inline float GetAspect() const						{ return aspect; }
	inline float GetNearPlane() const					{ return nearplane; }
	inline float GetFarPlane() const					{ return farplane; }
	inline float GetFov() const							{ return fov; }
	inline float GetSpeed() const						{ return speed; }

	inline bool IsAnimationFinished() const				{ return finished; }
};

#endif
