
#include "physicsworld.h"
#include "gl4ext.h"

const float PhysicsWorld::Immovable = 3.402823466e+38f;

class RigidSphere : public RigidBody
{
private:
	float radius;

public:
	RigidSphere(float radius);

	void GetTransformWithSize(Math::Matrix& out) override;

	inline float GetRadius() const	{ return radius; }
};

class RigidBox : public RigidBody
{
private:
	Math::Vector3 size;

public:
	RigidBox(float width, float height, float depth);

	void GetTransformWithSize(Math::Matrix& out) override;
	float RayIntersect(Math::Vector3& normal, const Math::Vector3& start, const Math::Vector3& dir) override;

	inline const float* GetSize() const	{ return size; }
};

// --- RigidSphere impl -------------------------------------------------------

RigidSphere::RigidSphere(float radius)
	: RigidBody(RigidBodyTypeSphere)
{
	this->radius = radius;
}

void RigidSphere::GetTransformWithSize(Math::Matrix& out)
{
	out = world;

	out._11 *= radius;
	out._12 *= radius;
	out._13 *= radius;

	out._21 *= radius;
	out._22 *= radius;
	out._23 *= radius;

	out._31 *= radius;
	out._32 *= radius;
	out._33 *= radius;

	out._41 -= (pivot[0] * out._11 + pivot[1] * out._21 + pivot[2] * out._31);
	out._42 -= (pivot[0] * out._12 + pivot[1] * out._22 + pivot[2] * out._32);
	out._43 -= (pivot[0] * out._13 + pivot[1] * out._23 + pivot[2] * out._33);
}

// --- RigidBox impl ----------------------------------------------------------

RigidBox::RigidBox(float width, float height, float depth)
	: RigidBody(RigidBodyTypeBox)
{
	size = Math::Vector3(width, height, depth);
}

void RigidBox::GetTransformWithSize(Math::Matrix& out)
{
	out = world;

	out._11 *= size[0];
	out._12 *= size[0];
	out._13 *= size[0];

	out._21 *= size[1];
	out._22 *= size[1];
	out._23 *= size[1];

	out._31 *= size[2];
	out._32 *= size[2];
	out._33 *= size[2];

	out._41 -= pivot[0];
	out._42 -= pivot[1];
	out._43 -= pivot[2];
}

float RigidBox::RayIntersect(Math::Vector3& normal, const Math::Vector3& start, const Math::Vector3& dir)
{
	Math::AABox bb(size);
	Math::Vector3 s, d, p, q;
	Math::Vector3 center, halfsize;

	Math::Vec3TransformCoord(s, start, worldinv);
	Math::Vec3TransformNormalTranspose(d, world, dir);

	Math::Vec3Subtract(bb.Min, bb.Min, pivot);
	Math::Vec3Subtract(bb.Max, bb.Max, pivot);

	float t = bb.RayIntersect(s, d);
	float mindist = FLT_MAX;
	float dist;

	bb.GetCenter(center);
	bb.GetHalfSize(halfsize);

	Math::Vec3Mad(p, s, d, t);
	Math::Vec3Subtract(q, p, center);

	if ((dist = fabs(halfsize[0] - q[0])) < mindist) {
		mindist = dist;
		normal = Math::Vector3((q[0] < 0.0f ? -1.0f : 1.0f), 0, 0);
	}

	if ((dist = fabs(halfsize[1] - q[1])) < mindist) {
		mindist = dist;
		normal = Math::Vector3(0, (q[1] < 0.0f ? -1.0f : 1.0f), 0);
	}

	if ((dist = fabs(halfsize[2] - q[2])) < mindist) {
		mindist = dist;
		normal = Math::Vector3(0, 0, (q[2] < 0.0f ? -1.0f : 1.0f));
	}

	Math::Vec3TransformNormalTranspose(normal, worldinv, normal);

	return t;
}

// --- RigidBody impl ---------------------------------------------------------

RigidBody::RigidBody(RigidBodyType bodytype)
{
	type = bodytype;
	invmass = 0;
	userdata = 0;

	Math::QuaternionIdentity(previous.orientation);
	Math::QuaternionIdentity(current.orientation);

	UpdateMatrices();
}

RigidBody::~RigidBody()
{
}

void RigidBody::GetInterpolatedPosition(Math::Vector3& out, float t) const
{
	out[0] = (1.0f - t) * previous.position[0] + t * current.position[0];
	out[1] = (1.0f - t) * previous.position[1] + t * current.position[1];
	out[2] = (1.0f - t) * previous.position[2] + t * current.position[2];
}

void RigidBody::GetTransformWithSize(Math::Matrix& out)
{
}

float RigidBody::RayIntersect(Math::Vector3& normal, const Math::Vector3& start, const Math::Vector3& dir)
{
	return FLT_MAX;
}

void RigidBody::UpdateMatrices()
{
	Math::MatrixRotationQuaternion(prevworld, previous.orientation);
	Math::MatrixRotationQuaternion(world, current.orientation);

	prevworld._41 = previous.position[0];
	prevworld._42 = previous.position[1];
	prevworld._43 = previous.position[2];

	world._41 = current.position[0];
	world._42 = current.position[1];
	world._43 = current.position[2];

	Math::MatrixInverse(worldinv, world);
	Math::MatrixInverse(prevworldinv, prevworld);
}

void RigidBody::Integrate(float dt)
{
	const Math::Vector3 gravity = { 0, -10, 0 };

	if (invmass == 0)
		return;

	previous.position = current.position;

	velocity[0] += gravity[0] * dt;
	velocity[1] += gravity[1] * dt;
	velocity[2] += gravity[2] * dt;

	current.position[0] += velocity[0] * dt;
	current.position[1] += velocity[1] * dt;
	current.position[2] += velocity[2] * dt;
}

void RigidBody::IntegratePosition(float dt)
{
	current.position[0] += velocity[0] * dt;
	current.position[1] += velocity[1] * dt;
	current.position[2] += velocity[2] * dt;
}

void RigidBody::ResolvePenetration(const Contact& contact)
{
	Math::Vec3Mad(current.position, current.position, contact.normal, contact.depth + 1e-3f);	// 1 mm
}

void RigidBody::ResolvePenetration(float toi)
{
	Math::Vec3Mad(current.position, previous.position, velocity, toi);
}

void RigidBody::SetMass(float mass)
{
	if (mass == PhysicsWorld::Immovable)
		invmass = 0;
	else
		invmass = 1.0f / mass;
}

void RigidBody::SetPivot(const Math::Vector3& offset)
{
	pivot = offset;
}

void RigidBody::SetPosition(float x, float y, float z)
{
	previous.position = Math::Vector3(x, y, z);
	current.position = Math::Vector3(x, y, z);

	UpdateMatrices();
}

void RigidBody::SetVelocity(float x, float y, float z)
{
	velocity = Math::Vector3(x, y, z);
}

void RigidBody::SetVelocity(const Math::Vector3& v)
{
	velocity = v;
}

void RigidBody::SetOrientation(const Math::Quaternion& q)
{
	previous.orientation = q;
	current.orientation = q;

	UpdateMatrices();
}

// --- PhysicsWorld impl ------------------------------------------------------

PhysicsWorld::PhysicsWorld()
{
	memset(detectors, 0, sizeof(detectors));

	detectors[1][2] = &PhysicsWorld::SphereSweepBox;
	detectors[2][1] = &PhysicsWorld::BoxSweepSphere;
}

PhysicsWorld::~PhysicsWorld()
{
	for (size_t i = 0; i < bodies.size(); ++i)
		delete bodies[i];

	bodies.clear();
}

RigidBody* PhysicsWorld::AddStaticBox(float width, float height, float depth)
{
	RigidBody* body = new RigidBox(width, height, depth);

	body->SetMass(Immovable);
	bodies.push_back(body);

	return body;
}

RigidBody* PhysicsWorld::AddDynamicSphere(float radius, float mass)
{
	RigidBody* body = new RigidSphere(radius);

	body->SetMass(mass);
	bodies.push_back(body);

	return body;
}

RigidBody* PhysicsWorld::RayIntersect(const Math::Vector3& start, const Math::Vector3& dir)
{
	Math::Vector4 params;
	return RayIntersect(params, start, dir);
}

RigidBody* PhysicsWorld::RayIntersect(Math::Vector4& out, const Math::Vector3& start, const Math::Vector3& dir)
{
	RigidBody* bestbody = nullptr;
	float t, bestt = FLT_MAX;
	Math::Vector3 n;

	// TODO: kD-tree
	for (size_t i = 0; i < bodies.size(); ++i) {
		t = bodies[i]->RayIntersect(n, start, dir);

		if (t < bestt) {
			bestt = t;
			bestbody = bodies[i];

			out.x = n.x;
			out.y = n.y;
			out.z = n.z;
		}
	}

	out[3] = bestt;
	return bestbody;
}

bool PhysicsWorld::SphereSweepBox(CollisionData& out, RigidBody* body1, RigidBody* body2)
{
	RigidSphere*	sphere		= (RigidSphere*)body1;
	RigidBox*		box			= (RigidBox*)body2;

	Math::AABox		inner(box->GetSize());
	Contact			contact;
	Math::Vector3	start;
	Math::Vector3	v1, v2;
	Math::Vector3	rel_vel;

	contact.body1 = body1;
	contact.body2 = body2;

	// calculate relative velocity
	Math::Vec3Subtract(v1, box->current.position, box->previous.position);
	Math::Vec3Subtract(v2, sphere->current.position, sphere->previous.position);
	Math::Vec3Subtract(rel_vel, v2, v1);

	if (Math::Vec3Dot(rel_vel, rel_vel) < 1e-5f)
		return false;

	// transform to box space
	Math::Vec3TransformCoord(start, sphere->previous.position, box->worldinv);
	Math::Vec3TransformNormal(rel_vel, rel_vel, box->worldinv);

	Math::Vec3Subtract(inner.Min, inner.Min, box->pivot);
	Math::Vec3Subtract(inner.Max, inner.Max, box->pivot);

	Math::Vector3	estpos;
	Math::Vector3	closest;
	Math::Vector4	worldplane;
	Math::Vector3	norm;
	Math::Vector3	tmp;

	float			dist, prevdist;
	float			maxdist = Math::Vec3Length(rel_vel);
	float			dt, t = 0;
	int				numiter = 0;

	estpos = start;

	closest[0] = Math::Clamp(estpos[0], inner.Min[0], inner.Max[0]);
	closest[1] = Math::Clamp(estpos[1], inner.Min[1], inner.Max[1]);
	closest[2] = Math::Clamp(estpos[2], inner.Min[2], inner.Max[2]);

	dist = Math::Vec3Distance(estpos, closest) - sphere->GetRadius();

	if (maxdist > 1e-3f) {
		// conservative advancement
		while (numiter < 15) {
			prevdist = dist;
			dt = dist / maxdist;

			Math::Vec3Scale(tmp, rel_vel, dt);
			Math::Vec3Add(estpos, estpos, tmp);

			t += dt;

			closest[0] = Math::Clamp(estpos[0], inner.Min[0], inner.Max[0]);
			closest[1] = Math::Clamp(estpos[1], inner.Min[1], inner.Max[1]);
			closest[2] = Math::Clamp(estpos[2], inner.Min[2], inner.Max[2]);

			dist = Math::Vec3Distance(estpos, closest) - sphere->GetRadius();

			if (dist >= prevdist || t > 1 || dist < 1e-3f)	// 1 mm
				break;

			++numiter;
		}
	}

	if (dist < 1e-3f) {
		const Math::Vector4 planes[6] = {
			{ 1, 0, 0, -inner.Max[0] },
			{ -1, 0, 0, -inner.Min[0] },
			{ 0, 1, 0, -inner.Max[1]},
			{ 0, -1, 0, -inner.Min[1]},
			{ 0, 0, 1, -inner.Max[2]},
			{ 0, 0, -1, -inner.Min[2]}
		};

		Math::Vec3Subtract(norm, estpos, closest);

		if (Math::Vec3Dot(norm, norm) < 1e-5f) {
			// find plane by hand
			maxdist = -FLT_MAX;

			for (int i = 0; i < 6; ++i) {
				dist = Math::PlaneDotCoord(planes[i], estpos);

				if (fabs(dist) < sphere->GetRadius()) {
					worldplane = planes[i];
					break;
				}
			}
		} else {
			Math::PlaneFromNormalAndPoint(worldplane, norm, closest);
		}

		Math::PlaneTransformTranspose(worldplane, box->worldinv, worldplane);
		Math::PlaneNormalize(worldplane, worldplane);

		contact.toi = t;

		// convert to current frame
		t = Math::PlaneDotCoord(worldplane, sphere->current.position);
		contact.depth = sphere->GetRadius() - t;

		contact.normal = (const Math::Vector3&)worldplane;

		// contact on sphere
		Math::Vec3Scale(contact.pos1, contact.normal, sphere->GetRadius());
		Math::Vec3Subtract(contact.pos1, sphere->current.position, contact.pos1);

		// contact on box
		Math::Vec3Scale(contact.pos2, contact.normal, t);
		Math::Vec3Subtract(contact.pos2, sphere->current.position, contact.pos2);

		out.contacts.push_back(contact);
		return true;
	}

	return false;
}

bool PhysicsWorld::BoxSweepSphere(CollisionData& out, RigidBody* body1, RigidBody* body2)
{
	bool collided = SphereSweepBox(out, body2, body1);

	if (out.contacts.size() > 0) {
		Contact& contact = out.contacts[0];

		Math::Vec3Scale(contact.normal, contact.normal, -1);
		Math::Swap(contact.pos1, contact.pos2);
		Math::Swap(contact.body1, contact.body2);
	}

	return collided;
}

bool PhysicsWorld::Detect(CollisionData& out, RigidBody* body1, RigidBody* body2)
{
	DetectorFunc func = detectors[body1->GetType()][body2->GetType()];

	if (func != nullptr)
		return (this->*func)(out, body1, body2);

	return false;
}

void PhysicsWorld::DetectCollisions(CollisionData& out, RigidBody* body)
{
	for (size_t i = 0; i < bodies.size(); ++i) {
		if (bodies[i] != body)
			Detect(out, body, bodies[i]);
	}
}

void PhysicsWorld::DEBUG_Visualize(void (*callback)(RigidBodyType, const Math::Matrix&))
{
	if (!callback)
		return;

	Math::Matrix xform;

	for (size_t i = 0; i < bodies.size(); ++i) {
		bodies[i]->GetTransformWithSize(xform);
		(*callback)(bodies[i]->GetType(), xform);
	}
}
