#include "pch.h"
#include "CollisionShape.h"

/*---------------[ BoxShape ]---------------*/

CollisionShape::CollisionShape(ShapeType shape, CollisionType type, const vec3& offset)
    : _shape(shape), _type(type), _localOffset(offset)
{
    if (_type == CollisionType::Hurt) {
        _active = true;
    }

    else {
        _active = false;
    }
}

bool BoxShape::CheckCollision(const CollisionShape& other) const
{
    return false;
}

/*---------------[ SphereShape ]---------------*/

bool SphereShape::CheckCollision(const CollisionShape& other) const
{
    return false;
}

/*---------------[ CylinderShape ]---------------*/

bool CylinderShape::CheckCollision(const CollisionShape& other) const
{
    return false;
}



bool Collision::CheckBoxVsBox(const BoxShape& a, const BoxShape& b)
{
    return false;
}

bool Collision::CheckBoxVsSphere(const BoxShape& a, const SphereShape& b)
{
    return false;
}

bool Collision::CheckBoxVsCylinder(const BoxShape& a, const CylinderShape& b)
{
    return false;
}

bool Collision::CheckSphereVsSphere(const SphereShape& a, const SphereShape& b)
{
    return false;
}

bool Collision::CheckSphereVsCylinder(const SphereShape& a, const CylinderShape& b)
{
    return false;
}

bool Collision::CheckCylinderVsCylinder(const CylinderShape& a, const CylinderShape& b)
{
    return false;
}
