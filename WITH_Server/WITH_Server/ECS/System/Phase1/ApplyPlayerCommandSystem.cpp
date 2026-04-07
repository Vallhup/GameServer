#include "pch.h"
#include "ApplyPlayerCommandSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ApplyPlayerCommandSystem::kMeta =
	MakeSystemMeta<ApplyPlayerCommandSystem>("ApplyPlayerCommandSystem");

void ApplyPlayerCommandSystem::Execute(SystemContext& ctx)
{
	for (const WorldCommand& command : ctx.runtime.GetFrameWorldCommands())
	{
		Entity targetEntity = Entity::Null();
		for (const auto& [entity, identity] : ctx.ecs.View<PlayerControlIdentityComp>())
		{
			if (identity.netId == command.targetNetId)
			{
				targetEntity = entity;
				break;
			}
		}

		if (targetEntity.IsNull())
		{
			continue;
		}

		auto* identity = MutableComponent<PlayerControlIdentityComp>(
			ctx.ecs,
			targetEntity);
		auto* input = MutableComponent<ActorInputComp>(ctx.ecs, targetEntity);
		if (identity == nullptr || input == nullptr)
		{
			continue;
		}

		if (identity->ownerSessionId != static_cast<SessionId>(command.sourceId))
		{
			continue;
		}

		if (IsPlayerMoveCommandType(command.typeKey))
		{
			PlayerMoveCommandPayload payload{};
			if (!TryDecodePlayerMoveCommandPayload(command, payload))
			{
				continue;
			}

			input->move.inputX = ClampFloat(payload.inputX, -1.0f, 1.0f);
			input->move.inputZ = ClampFloat(payload.inputZ, -1.0f, 1.0f);
			input->move.cameraYawRad = WrapYaw(payload.yaw);
			input->move.wantsRun = payload.isRun != 0;
			input->move.lastUpdatedFrame = ctx.runtime.FrameIndex();
			continue;
		}

		if (IsPlayerActionEventCommandType(command.typeKey))
		{
			PlayerDirectionCommandPayload payload{};
			if (!TryDecodePlayerDirectionCommandPayload(command, payload))
			{
				continue;
			}

			input->action.directionX = payload.dirX;
			input->action.directionZ = payload.dirZ;
			input->action.requestedFrame = ctx.runtime.FrameIndex();

			switch (static_cast<PlayerCommandTypeKey>(command.typeKey)) {
			case PlayerCommandTypeKey::LightAttack:
			{
				input->action.type = PlayerActionInputType::LightAttack;
				break;
			}
			case PlayerCommandTypeKey::HeavyAttack:
			{
				input->action.type = PlayerActionInputType::HeavyAttack;
				break;
			}
			case PlayerCommandTypeKey::Dodge:
			{
				input->action.type = PlayerActionInputType::Dodge;
				break;
			}
			case PlayerCommandTypeKey::Parry:
			{
				input->action.type = PlayerActionInputType::Parry;
				break;
			}
			default:
			{
				input->action.type = PlayerActionInputType::None;
				break;
			}
			}
			continue;
		}

		if (IsPlayerGuardCommandType(command.typeKey))
		{
			PlayerGuardCommandPayload payload{};
			if (!TryDecodePlayerGuardCommandPayload(command, payload))
			{
				continue;
			}

			input->guard.isPressed = payload.pressed != 0;
			input->guard.lastUpdatedFrame = ctx.runtime.FrameIndex();
		}
	}
}
