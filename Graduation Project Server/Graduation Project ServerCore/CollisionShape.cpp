#include "pch.h"
#include "CollisionShape.h"

bool BoxShape::CheckCollision(const CollisionShape& other) const
{
    return false;
}

bool SphereShape::CheckCollision(const CollisionShape& other) const
{
    return false;
}

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
