#pragma once

enum class ShapeType : char { Box, Sphere, Cylinder };
enum class CollisionType : char { Attack, Hurt, Parry };

class CollisionShape {
public:
	CollisionShape() = delete;
	CollisionShape(ShapeType shape, CollisionType type, const vec3& offset);
	virtual ~CollisionShape() = default;

protected:
	CollisionShape(const CollisionShape& other) = delete;
	CollisionShape& operator=(const CollisionShape& other) = delete;

public:
	virtual bool CheckCollision(const CollisionShape& other) const = 0;
	virtual std::unique_ptr<CollisionShape> Clone() const = 0;

public:
	ShapeType GetShape() const { return _shape; }
	CollisionType GetCollisionType() const { return _type; }
	const vec3& GetLocalOffset() const { return _localOffset; }
	bool IsActive() const { return _active; }

	void SetActive(bool active) { _active = active; }

protected:
	ShapeType _shape;
	CollisionType _type;
	vec3 _localOffset;
	bool _active;
};

class BoxShape : public CollisionShape {
public:
	BoxShape() = delete;
	BoxShape(CollisionType type, const vec3& offset, const vec3& halfSize) 
		: CollisionShape(ShapeType::Box, type, offset), _halfSize(halfSize) {}
	virtual ~BoxShape() = default;

	BoxShape(const BoxShape& other)
		: CollisionShape(other._shape, other._type, other._localOffset),
		_halfSize(other._halfSize) {}
	BoxShape& operator=(const BoxShape& other) = delete;

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;
	virtual std::unique_ptr<CollisionShape> Clone() const override;

public:
	const vec3& GetHalfSize() const { return _halfSize; }

private:
	vec3 _halfSize;
};

class SphereShape : public CollisionShape {
public:
	SphereShape() = delete;
	SphereShape(CollisionType type, const vec3& offset, float radius) 
		: CollisionShape(ShapeType::Sphere, type, offset), _radius(radius) {}
	virtual ~SphereShape() = default;

	SphereShape(const SphereShape& other)
		: CollisionShape(other._shape, other._type, other._localOffset),
		_radius(other._radius) {}
	SphereShape& operator=(const SphereShape& other) = delete;

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;
	virtual std::unique_ptr<CollisionShape> Clone() const override;

public:
	float GetRadius() const { return _radius; }

private:
	float _radius;
};

class CylinderShape : public CollisionShape {
public:
	CylinderShape() = delete;
	CylinderShape(CollisionType type, const vec3& offset, float radius, float height, const vec3& direction)
		: CollisionShape(ShapeType::Cylinder, type, offset), _radius(radius), _height(height), _direction(direction) {}
	virtual ~CylinderShape() = default;

	CylinderShape(const CylinderShape& other)
		: CollisionShape(other._shape, other._type, other._localOffset),
		_radius(other._radius), _height(other._height), _direction(other._direction) {}
	CylinderShape& operator=(const CylinderShape& other) = delete;

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;
	virtual std::unique_ptr<CollisionShape> Clone() const override;

public:
	float GetRadius() const { return _radius; }
	float GetHeight() const { return _height; }
	const vec3& GetDirection() const { return _direction; }

private:
	float _radius;
	float _height;
	vec3 _direction;
};

namespace Collision {
	bool CheckBoxVsBox(const BoxShape& a, const BoxShape& b);
	bool CheckBoxVsSphere(const BoxShape& a, const SphereShape& b);
	bool CheckBoxVsCylinder(const BoxShape& a, const CylinderShape& b);
	bool CheckSphereVsSphere(const SphereShape& a, const SphereShape& b);
	bool CheckSphereVsCylinder(const SphereShape& a, const CylinderShape& b);
	bool CheckCylinderVsCylinder(const CylinderShape& a, const CylinderShape& b);
}
