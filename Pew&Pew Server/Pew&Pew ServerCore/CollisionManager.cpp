#include "pch.h"
#include "CollisionManager.h"

bool CollisionManager::SphereAABBCollision(const vec3& sphereCenter, float sphereRadius, const vec3& minAABB, const vec3& maxAABB)
{
	vec3 closestPt = {
		std::max(minAABB.x, std::min(sphereCenter.x, maxAABB.x)),
		std::max(minAABB.y, std::min(sphereCenter.y, maxAABB.y)),
		std::max(minAABB.z, std::min(sphereCenter.z, maxAABB.z))
	};

	return sphereCenter.DistanceSq(closestPt) <= pow(sphereRadius, 2);
}

void CollisionManager::Update(const std::vector<std::shared_ptr<Projectile>>& projectiles, const std::vector<std::shared_ptr<Character>>& characters, Service* service)
{
	for (auto& projectile : projectiles) {
		for (auto& character : characters) {
			if (not character->IsAlive()) continue;
			if (projectile->GetOwnerId() == character->GetId()) continue;

			if (projectile->CheckCollision(*character)) {
				character->TakeDamage(projectile->GetDamage());
				projectile->SetIntactive(service);
				break;
				
				// projectile ªË¡¶
			}
		}
	}
}
