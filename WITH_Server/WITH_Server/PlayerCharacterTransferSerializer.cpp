#include "pch.h"
#include "PlayerCharacterTransferSerializer.h"

#include <cstring>
#include <type_traits>

#include "Aspect/CharacterAspectRegistry.h"
#include "Aspect/ICharacterAspect.h"
#include "CharacterDef.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "IWorldTransferSerializer.h"
#include "WorldRuntime.h"

namespace
{
	constexpr WorldTransferSerializerId kPlayerCharacterTransferSerializerId = 1;

	struct PlayerCharacterTransferPayload
	{
		CharacterId characterId{ CharacterId::None };
		DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		CombatStatInitialState combatStats{};
		bool hasCombatStats{ false };
	};

	static_assert(
		std::is_trivially_copyable_v<PlayerCharacterTransferPayload>,
		"PlayerCharacterTransferPayload must stay trivially copyable.");

	class PlayerCharacterTransferSerializer final
		: public IWorldTransferSerializer {
	public:
		WorldTransferSerializerId GetSerializerId() const noexcept override
		{
			return kPlayerCharacterTransferSerializerId;
		}

		bool Export(
			const WorldTransferExportContext& context,
			std::vector<std::byte>& outBytes) const override
		{
			outBytes.clear();

			const SpawnTypeComp* const spawn =
				context.sourceView.GetComponent<SpawnTypeComp>(
					context.sourceEntity);
			const WorldTransformComp* const transform =
				context.sourceView.GetComponent<WorldTransformComp>(
					context.sourceEntity);
			const PlayerControlIdentityComp* const player =
				context.sourceView.GetComponent<PlayerControlIdentityComp>(
					context.sourceEntity);
			if (spawn == nullptr ||
				transform == nullptr ||
				player == nullptr ||
				!player->netId.IsValid() ||
				!context.netId.IsValid() ||
				player->netId != context.netId ||
				player->ownerSessionId == 0 ||
				player->ownerSessionId != context.sessionId)
			{
				return false;
			}

			PlayerCharacterTransferPayload payload{};
			payload.characterId = spawn->characterId;
			payload.position = transform->position;
			payload.rotation = transform->rotation;

			if (const CombatStatStateComp* const stats =
				context.sourceView.GetComponent<CombatStatStateComp>(
					context.sourceEntity))
			{
				payload.combatStats = ToCombatStatInitialState(*stats);
				payload.hasCombatStats = true;
			}

			outBytes.resize(sizeof(payload));
			std::memcpy(outBytes.data(), &payload, sizeof(payload));
			return true;
		}

		bool Import(
			const WorldTransferImportContext& context,
			std::span<const std::byte> bytes) const override
		{
			if (bytes.size() != sizeof(PlayerCharacterTransferPayload))
			{
				return false;
			}

			PlayerCharacterTransferPayload payload{};
			std::memcpy(&payload, bytes.data(), sizeof(payload));

			const CharacterDef* const characterDef =
				FindCharacterDef(payload.characterId);
			if (characterDef == nullptr || !characterDef->IsPlayable())
			{
				return false;
			}

			AssembleParams params{};
			params.position = context.hasSpawnTransformOverride
				? context.spawnPositionOverride
				: payload.position;
			params.rotation = context.hasSpawnTransformOverride
				? context.spawnRotationOverride
				: payload.rotation;
			params.netId = context.netId;
			params.sessionId = context.sessionId;
			if (payload.hasCombatStats)
			{
				params.combatStatsOverride = payload.combatStats;
			}

			GetGlobalCharacterAspectRegistry().Assemble(
				context.targetRuntime,
				context.targetEntity,
				*characterDef,
				params);
			return !context.targetRuntime.IsFaulted();
		}

	private:
		static CombatStatInitialState ToCombatStatInitialState(
			const CombatStatStateComp& stats) noexcept
		{
			CombatStatInitialState initial{};
			initial.currentHp = stats.currentHp;
			initial.maxHp = stats.maxHp;
			initial.currentStamina = stats.currentStamina;
			initial.maxStamina = stats.maxStamina;
			initial.currentPoise = stats.currentPoise;
			initial.maxPoise = stats.maxPoise;
			initial.attackPower = stats.attackPower;
			initial.defense = stats.defense;
			initial.attackSpeed = stats.attackSpeed;
			initial.moveSpeed = stats.moveSpeed;
			return initial;
		}
	};
}

std::unique_ptr<IWorldTransferSerializer> CreatePlayerCharacterTransferSerializer()
{
	return std::make_unique<PlayerCharacterTransferSerializer>();
}
