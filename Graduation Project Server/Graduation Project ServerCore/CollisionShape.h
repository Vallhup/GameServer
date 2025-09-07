#pragma once

enum class ShapeType : char { Box, Sphere, Cylinder };
enum class CollisionType : char { Attack, Hurt, Parry };

class CollisionShape {
public:
	CollisionShape() = delete;
	CollisionShape(ShapeType shape, CollisionType type, const vec3& offset);
	virtual ~CollisionShape() = default;

public:
	virtual bool CheckCollision(const CollisionShape& other) const = 0;

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

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;

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

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;

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

public:
	virtual bool CheckCollision(const CollisionShape& other) const override;

public:
	float GetRadius() const { return _radius; }
	float GetHeight() const { return _height; }
	const vec3& GetDirection() const { return _direction; }

private:
	float _radius;
	float _height;
	vec3 _direction;
};

// 실제 충돌처리를 Client or Server중 어디서 처리할지도 고려해야 됨
//
// 1. Client 처리 + Server 검증
//  - Server에서 최소한의 무결성 검증만 진행
//  - 지연 거의 X, Animation Frame에 따라 정확한 충돌 처리 가능
// 
// 2. Server 처리
//  - 약간의 지연 발생 (Client에서 예측, 보정해줘야 됨)
//  - 처리 일관성 (모든 Logic은 Server에서 처리하는 원칙)

namespace Collision {
	bool CheckBoxVsBox(const BoxShape& a, const BoxShape& b);
	bool CheckBoxVsSphere(const BoxShape& a, const SphereShape& b);
	bool CheckBoxVsCylinder(const BoxShape& a, const CylinderShape& b);
	bool CheckSphereVsSphere(const SphereShape& a, const SphereShape& b);
	bool CheckSphereVsCylinder(const SphereShape& a, const CylinderShape& b);
	bool CheckCylinderVsCylinder(const CylinderShape& a, const CylinderShape& b);
}
