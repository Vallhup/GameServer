#include "pch.h"
#include "CharacterDataService.h"

#include "GameDataCatalog.h"
#include "FrameworkRuntime.h"
#include "WorldDef.h"
#include "WorldInstance.h"

namespace
{
	CharacterDataResult MakeResult(
		CharacterDataResultCode code,
		SessionId sessionId,
		CharacterId characterId = CharacterId::None,
		WorldId worldId = WorldId::Invalid(),
		const CharacterDef* characterDef = nullptr) noexcept
	{
		CharacterDataResult result{};
		result.code = code;
		result.sessionId = sessionId;
		result.characterId = characterId;
		result.worldId = worldId;
		result.characterDef = characterDef;
		return result;
	}

	bool TryResolveDefaultPlayerSpawnTransform(
		const WorldDef& worldDef,
		DirectX::XMFLOAT3& outPosition,
		DirectX::XMFLOAT4& outRotation) noexcept
	{
		if (worldDef.map.defaultPlayerSpawnPointId == SpawnPointIds::None)
		{
			return false;
		}

		for (const SpawnPointDef& spawnPoint : worldDef.map.spawnPoints)
		{
			if (spawnPoint.id != worldDef.map.defaultPlayerSpawnPointId)
			{
				continue;
			}

			outPosition = DirectX::XMFLOAT3{
				spawnPoint.position.x,
				spawnPoint.position.y,
				spawnPoint.position.z
			};
			outRotation = DirectX::XMFLOAT4{
				spawnPoint.rotation.x,
				spawnPoint.rotation.y,
				spawnPoint.rotation.z,
				spawnPoint.rotation.w
			};
			return true;
		}

		return false;
	}
}

CharacterDataService::CharacterDataService(Dependencies deps)
	: _deps(deps)
{
}

void CharacterDataService::SetDependencies(Dependencies deps) noexcept
{
	_deps = deps;
}

CharacterDataResult CharacterDataService::ResolveCharacterSelect(
	SessionId sessionId,
	CharacterId characterId) const
{
	if (sessionId == 0)
	{
		return MakeResult(CharacterDataResultCode::InvalidSession, sessionId, characterId);
	}

	const CharacterDef* const characterDef =
		GameDataCatalog::Current().Characters().Find(characterId);
	if (characterDef == nullptr)
	{
		return MakeResult(CharacterDataResultCode::CharacterDefNotFound, sessionId, characterId);
	}

	if (!characterDef->IsPlayable())
	{
		return MakeResult(CharacterDataResultCode::CharacterIdNotPlayable, sessionId, characterId);
	}

	if (_deps.framework == nullptr ||
		_deps.startupWorldId == nullptr ||
		!_deps.startupWorldId->IsValid())
	{
		return MakeResult(CharacterDataResultCode::StartupWorldUnavailable, sessionId, characterId);
	}

	const WorldId startupWorldId = *_deps.startupWorldId;
	const WorldInstance* const world = _deps.framework->FindWorld(startupWorldId);
	if (world == nullptr)
	{
		return MakeResult(
			CharacterDataResultCode::WorldNotFound,
			sessionId,
			characterId,
			startupWorldId,
			characterDef);
	}

	CharacterDataResult result = MakeResult(
		CharacterDataResultCode::Success,
		sessionId,
		characterDef->id,
		startupWorldId,
		characterDef);

	if (const WorldDef* const worldDef = world->GetDef(); worldDef != nullptr)
	{
		(void)TryResolveDefaultPlayerSpawnTransform(
			*worldDef,
			result.spawnPosition,
			result.spawnRotation);
	}

	return result;
}
