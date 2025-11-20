#include "pch.h"
#include "CollisionShape.h"

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

/*---------------[ BoxShape ]---------------*/

std::unique_ptr<CollisionShape> BoxShape::ToWorldShape(const TransformComponent& trComp) const
{
    return std::unique_ptr<CollisionShape>();
}

bool BoxShape::CheckCollision(CollisionShape& other) const
{
    return false;
}

std::unique_ptr<CollisionShape> BoxShape::Clone() const
{
	return std::make_unique<BoxShape>(*this);
}

/*---------------[ SphereShape ]---------------*/

std::unique_ptr<CollisionShape> SphereShape::ToWorldShape(const TransformComponent& trComp) const
{
    return std::unique_ptr<CollisionShape>();
}

bool SphereShape::CheckCollision(CollisionShape& other) const
{
    return false;
}

std::unique_ptr<CollisionShape> SphereShape::Clone() const
{
    return std::make_unique<SphereShape>(*this);
}

/*---------------[ CylinderShape ]---------------*/

std::unique_ptr<CollisionShape> CylinderShape::ToWorldShape(const TransformComponent& trComp) const
{
    auto RotateY =
        [](const vec3& v, float yaw) -> vec3
        {
            float c = cosf(yaw);
            float s = sinf(yaw);

            return vec3{
                v.x * c + v.z * s,
                v.y,
                -v.x * s + v.z * c
            };
        };

    vec3 worldOffset = trComp.GetPosition() + RotateY(_localOffset, trComp.GetAngle());
    vec3 worldDir = RotateY(_direction, trComp.GetAngle()).Normalize();

    return std::make_unique<CylinderShape>(_type, worldOffset, GetRadius(), GetHeight(), worldDir);
}

bool CylinderShape::CheckCollision(CollisionShape& other) const
{
    CylinderShape* other_ = static_cast<CylinderShape*>(&other);
    return Collision::CheckCylinderVsCylinder(*this, *other_);
}

std::unique_ptr<CollisionShape> CylinderShape::Clone() const
{
    return std::make_unique<CylinderShape>(*this);
}

void CylinderShape::Print(const char* name) const
{
    std::cout << "=== " << name << " (Cylinder) ===\n";
    std::cout << " type      = " << (int)_type << "\n";
    std::cout << " radius    = " << _radius << "\n";
    std::cout << " height    = " << _height << "\n";
    std::cout << " localOff  = (" << _localOffset.x << ", " << _localOffset.y << ", " << _localOffset.z << ")\n";
    std::cout << " direction = (" << _direction.x << ", " << _direction.y << ", " << _direction.z << ")\n";
    std::cout << " p0(local) = (" << _endPoints[0].x << ", " << _endPoints[0].y << ", " << _endPoints[0].z << ")\n";
    std::cout << " p1(local) = (" << _endPoints[1].x << ", " << _endPoints[1].y << ", " << _endPoints[1].z << ")\n";
    std::cout << "=============================\n";
}

//---------------[ Collision ]---------------*/

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

bool Collision::CheckCylinderVsCylinder(const CylinderShape& c1, const CylinderShape& c2)
{
    // TEMP : 두 캡슐 간의 충돌 검사로 근사
    // TODO : DXMath 사용한 최적화 필요

    const vec3* c1EndPoints = c1.GetEndPoints();
    const vec3* c2EndPoints = c2.GetEndPoints();

	const vec3 u = c1EndPoints[1] - c1EndPoints[0];
	const vec3 v = c2EndPoints[1] - c2EndPoints[0];
	const vec3 w = c1EndPoints[0] - c2EndPoints[0];

    const float a = u.Dot(u);
    const float b = u.Dot(v);
    const float c = v.Dot(v);
    const float d = u.Dot(w);
    const float e = v.Dot(w);

    const float D = a * c - b * b;
    float s, t;

    // D 값이 0에 가까우면 두 원기둥이 거의 평행한 상태
    if (D > 0.00001f) {
        s = (b * e - c * d) / D;
        t = (a * e - b * d) / D;

        if (s < 0.0f) {
            s = 0.0f;
            t = std::clamp<float>(e / c, 0.0f, 1.0f);
        }

        else if (s > 1.0f) {
            s = 1.0f;
            t = std::clamp<float>((e + b) / c, 0.0f, 1.0f);
		}

        if (t < 0.0f) {
            t = 0.0f;
            s = std::clamp<float>(-d / a, 0.0f, 1.0f);
        }

        else if (t > 1.0f) {
            t = 1.0f;
            s = std::clamp<float>((b - d) / a, 0.0f, 1.0f);
        }
    }

    else {
        // 점-선분 거리 계산
        // 나중에 별도 헬퍼 함수로 분리 고려
        auto PointToSegmentDistSq = 
            [](const vec3& P, const vec3& A, const vec3& B) -> float
            {
                const vec3 AB = B - A;
                const vec3 AP = P - A;

                const float abLenSq = AB.Dot(AB);
                if (abLenSq < 0.00001f) {
                    return (P - A).Dot(P - A);
                }

                float t = AP.Dot(AB) / abLenSq;
                t = std::clamp<float>(t, 0.0f, 1.0f);

                const vec3 closest = A + AB * t;
                const vec3 diff = P - closest;
                return diff.Dot(diff);
            };

        const float d1 = PointToSegmentDistSq(c1EndPoints[0], c2EndPoints[0], c2EndPoints[1]);
        const float d2 = PointToSegmentDistSq(c1EndPoints[1], c2EndPoints[0], c2EndPoints[1]);
        const float d3 = PointToSegmentDistSq(c2EndPoints[0], c1EndPoints[0], c1EndPoints[1]);
        const float d4 = PointToSegmentDistSq(c2EndPoints[1], c1EndPoints[0], c1EndPoints[1]);
        
        const float minDistSq = std::min({ d1, d2, d3, d4 });
        const float radius = c1.GetRadius() + c2.GetRadius();

        return minDistSq <= radius * radius;
    }

    const vec3 Pc = c1EndPoints[0] + u * s;
	const vec3 Qc = c2EndPoints[0] + v * t;
	const vec3 dP = Pc - Qc;

    const float distSq = dP.Dot(dP);
    const float radius = c1.GetRadius() + c2.GetRadius();

	return distSq <= radius * radius;
}
