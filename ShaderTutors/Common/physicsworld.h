
#ifndef _PHYSICSWORLD_H_
#define _PHYSICSWORLD_H_

#include <vector>
#include "3Dmath.h"

class RigidBody;

enum RigidBodyType
{
	RigidBodyTypeNone = 0,
	RigidBodyTypeSphere,
	RigidBodyTypeBox
};

struct Contact
{
	Math::Vector3	pos1;
	Math::Vector3	pos2;
	Math::Vector3	normal;
	RigidBody*		body1;
	RigidBody*		body2;
	float			depth;
	float			toi;	// time of impact
};

struct CollisionData
{
	std::vector<Contact> contacts;
};

class RigidBody
{
	friend class PhysicsWorld;

	struct Internal_State
	{
		Math::Quaternion orientation;
		Math::Vector3 position;
	};

protected:
	Internal_State	previous;
	Internal_State	current;

	Math::Matrix	prevworld;
	Math::Matrix	prevworldinv;
	Math::Matrix	world;
	Math::Matrix	worldinv;

	Math::Vector3	velocity;
	Math::Vector3	pivot;

	void*			userdata;
	float			invmass;
	RigidBodyType	type;

	RigidBody(RigidBodyType bodytype);

	void UpdateMatrices();

public:
	virtual ~RigidBody();

	virtual void GetTransformWithSize(Math::Matrix& out);
	virtual float RayIntersect(Math::Vector3& normal, const Math::Vector3& start, const Math::Vector3& dir);

	void GetInterpolatedPosition(Math::Vector3& out, float t) const;
	
	void Integrate(float dt);
	void IntegratePosition(float dt);
	void ResolvePenetration(const Contact& contact);
	void ResolvePenetration(float toi);

	void SetMass(float mass);
	void SetPivot(const Math::Vector3& offset);
	void SetPosition(float x, float y, float z);
	void SetVelocity(float x, float y, float z);
	void SetVelocity(const Math::Vector3& v);
	void SetOrientation(const Math::Quaternion& q);

	inline void SetUserData(void* data)						{ userdata = data; }
	inline void* GetUserData()								{ return userdata; }

	inline const Math::Vector3& GetVelocity() const			{ return velocity; }
	inline const Math::Vector3& GetPosition() const			{ return current.position; }
	inline const Math::Vector3& GetPreviousPosition() const	{ return previous.position; }
	inline const Math::Matrix& GetTransform() const			{ return world; }
	inline const Math::Matrix& GetInverseTransform() const	{ return worldinv; }
	inline RigidBodyType GetType() const					{ return type; }
};

class PhysicsWorld
{
	typedef std::vector<RigidBody*> BodyList;

	typedef bool (PhysicsWorld::*DetectorFunc)(CollisionData&, RigidBody*, RigidBody*);

private:
	DetectorFunc	detectors[3][3];
	BodyList		bodies;

	bool SphereSweepBox(CollisionData& out, RigidBody* body1, RigidBody* body2);
	bool BoxSweepSphere(CollisionData& out, RigidBody* body1, RigidBody* body2);
	bool Detect(CollisionData& out, RigidBody* body1, RigidBody* body2);

public:
	static const float Immovable;

	PhysicsWorld();
	~PhysicsWorld();

	RigidBody* AddStaticBox(float width, float height, float depth);
	RigidBody* AddDynamicSphere(float radius, float mass);
	RigidBody* RayIntersect(const Math::Vector3& start, const Math::Vector3& dir);
	RigidBody* RayIntersect(Math::Vector4& out, const Math::Vector3& start, const Math::Vector3& dir);

	void DetectCollisions(CollisionData& out, RigidBody* body);

	void DEBUG_Visualize(void (*callback)(RigidBodyType, const Math::Matrix&));
};

#endif
