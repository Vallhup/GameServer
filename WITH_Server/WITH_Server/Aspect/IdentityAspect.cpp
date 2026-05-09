#include "pch.h"
#include "IdentityAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "WorldRuntime.h"

CharacterFeatureFlags IdentityAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::None;
}

void IdentityAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<SpawnTypeComp>();
	runtime.RegisterStorage<WorldTransformComp>();

	// 캐릭터 공통 생명주기/전이 시그널 (시스템이 런타임에 동적 부착).
	// 어떤 캐릭터든 생성/파괴/월드이동 대상이 되므로 IdentityAspect 가 소유.
	runtime.RegisterStorage<PendingDespawnTag>();
	runtime.RegisterStorage<PendingWorldTransferTag>();
	runtime.RegisterStorage<PendingWorldTransferComp>();
}

void IdentityAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredUpsertComponent<SpawnTypeComp>(entity,
		SpawnTypeComp
		{ 
			.characterId = def.id 
		});

	runtime.DeferredUpsertComponent<WorldTransformComp>(entity, 
		WorldTransformComp
		{
			.position = params.position,
			.rotation = params.rotation
		});
}
