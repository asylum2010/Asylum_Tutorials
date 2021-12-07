
#ifndef _BASICCAMERA_H_
#define _BASICCAMERA_H_

#include "3Dmath.h"

class BasicCamera
{
	typedef Math::InterpolationArray<float, 1> CurveValue;
	typedef Math::InterpolationArray<float, 3> CurveVector;

private:
	CurveVector		anglecurve;
	CurveVector		pancurve;
	CurveValue		zoomcurve;

	Math::Vector3	targetangles;
	Math::Vector3	smoothedangles;
	Math::Vector3	targetpan;
	Math::Vector3	position;		// orbit point
	float			targetzoom;

	float			distance;		// distance from lookat
	float			nearplane;
	float			farplane;
	float			minzoom;
	float			maxzoom;
	float			minpitch;
	float			maxpitch;
	float			fov;
	float			aspect;
	bool			finished;
	bool			inertial;

	void GetViewAndEye(Math::Matrix& view, Math::Vector3& eye) const;

public:
	BasicCamera();
	
	void OrbitRight(float angle);
	void OrbitUp(float angle);
	void RollRight(float angle);
	void PanRight(float offset);
	void PanUp(float offset);
	void Zoom(float offset);

	void GetViewMatrix(Math::Matrix& out) const;
	void GetProjectionMatrix(Math::Matrix& out) const;
	void GetPosition(Math::Vector3& out) const;
	void GetEyePosition(Math::Vector3& out) const;
	void GetOrientation(Math::Vector3& out) const;
	
	void Update(float dt);
	void Animate(float alpha);
	void SetOrientation(float yaw, float pitch, float roll);
	void SetPosition(float x, float y, float z);
	void SetDistance(float value);

	inline void SetAspect(float value)					{ aspect = value; }
	inline void SetFov(float value)						{ fov = value; }
	inline void SetClipPlanes(float znear, float zfar)	{ nearplane = znear; farplane = zfar; }
	inline void SetZoomLimits(float zmin, float zmax)	{ minzoom = zmin; maxzoom = zmax; }
	inline void SetPitchLimits(float pmin, float pmax)	{ minpitch = pmin; maxpitch = pmax; }
	inline void SetIntertia(bool enable)				{ inertial = enable; }

	inline float GetAspect() const						{ return aspect; }
	inline float GetDistance() const					{ return distance; }
	inline float GetNearPlane() const					{ return nearplane; }
	inline float GetFarPlane() const					{ return farplane; }
	inline float GetFov() const							{ return fov; }
	inline float GetMinZoom() const						{ return minzoom; }
	inline float GetMaxZoom() const						{ return maxzoom; }

	inline bool IsAnimationFinished() const				{ return finished; }
};

#endif
