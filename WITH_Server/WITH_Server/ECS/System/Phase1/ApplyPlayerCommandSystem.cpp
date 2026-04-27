#include "pch.h"
#include "ApplyPlayerCommandSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 4> kApplyPlayerCommandAccesses{
		ReadImmediate(ExternalRes<WorldCommand>()),
		ReadImmediate(ExternalRes<IWorldNetBindingResolver>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		WriteImmediate(ComponentRes<ActorInputComp>()),
	};

	PlayerActionInputType ResolveActionInputType(
		WorldCommandTypeKey typeKey) noexcept
	{
		switch (static_cast<PlayerCommandTypeKey>(typeKey)) {
		case PlayerCommandTypeKey::LightAttack:
			return PlayerActionInputType::LightAttack;

		case PlayerCommandTypeKey::HeavyAttack:
			return PlayerActionInputType::HeavyAttack;

		case PlayerCommandTypeKey::Dodge:
			return PlayerActionInputType::Dodge;

		case PlayerCommandTypeKey::Parry:
			return PlayerActionInputType::Parry;

		default:
			return PlayerActionInputType::None;
		}
	}

	bool TryResolveCommandTarget(
		SystemContext& ctx,
		const WorldCommand& command,
		PlayerControlIdentityComp*& outIdentity,
		ActorInputComp*& outInput)
	{
		outIdentity = nullptr;
		outInput = nullptr;

		const IWorldNetBindingResolver* const resolver =
			ctx.services.netBindingResolver;
		if (resolver == nullptr)
		{
			return false;
		}

		Entity targetEntity = Entity::Null();
		if (!resolver->TryResolveEntity(command.targetNetId, targetEntity) ||
			targetEntity.IsNull())
		{
			return false;
		}

		outIdentity =
			ctx.ecs.GetMutableComponent<PlayerControlIdentityComp>(targetEntity);
		outInput =
			ctx.ecs.GetMutableComponent<ActorInputComp>(targetEntity);
		return outIdentity != nullptr && outInput != nullptr;
	}

	bool IsAuthorizedCommand(
		const PlayerControlIdentityComp& identity,
		const WorldCommand& command) noexcept
	{
		return identity.ownerSessionId ==
			static_cast<SessionId>(command.sourceId);
	}

	bool HandleMoveCommand(
		SystemContext& ctx,
		const WorldCommand& command,
		ActorInputComp& input)
	{
		PlayerMoveCommandPayload payload{};
		if (!TryDecodePlayerMoveCommandPayload(command, payload))
		{
			return false;
		}

		input.move.inputX = ClampFloat(payload.inputX, -1.0f, 1.0f);
		input.move.inputZ = ClampFloat(payload.inputZ, -1.0f, 1.0f);
		input.move.cameraYawRad = WrapYaw(payload.yaw);
		input.move.wantsRun = payload.isRun != 0;
		input.move.lastUpdatedFrame = ctx.runtime.FrameIndex();
		return true;
	}

	bool HandleActionCommand(
		SystemContext& ctx,
		const WorldCommand& command,
		ActorInputComp& input)
	{
		PlayerDirectionCommandPayload payload{};
		if (!TryDecodePlayerDirectionCommandPayload(command, payload))
		{
			return false;
		}

		input.action.directionX = payload.dirX;
		input.action.directionZ = payload.dirZ;
		input.action.requestedFrame = ctx.runtime.FrameIndex();
		input.action.type = ResolveActionInputType(command.typeKey);
		return input.action.type != PlayerActionInputType::None;
	}

	bool HandleGuardCommand(
		SystemContext& ctx,
		const WorldCommand& command,
		ActorInputComp& input)
	{
		PlayerGuardCommandPayload payload{};
		if (!TryDecodePlayerGuardCommandPayload(command, payload))
		{
			return false;
		}

		input.guard.isPressed = payload.pressed != 0;
		input.guard.lastUpdatedFrame = ctx.runtime.FrameIndex();
		return true;
	}
}

const SystemMeta ApplyPlayerCommandSystem::kMeta =
	SystemMeta{
		SysTag<ApplyPlayerCommandSystem>(),
		"ApplyPlayerCommandSystem",
		kApplyPlayerCommandAccesses,
		kNoDeps,
		kNoDeps
	};

void ApplyPlayerCommandSystem::Execute(SystemContext& ctx)
{
	for (const WorldCommand& command : ctx.runtime.GetFrameWorldCommands())
	{
		PlayerControlIdentityComp* identity = nullptr;
		ActorInputComp* input = nullptr;
		if (!TryResolveCommandTarget(ctx, command, identity, input))
		{
			continue;
		}

		if (!IsAuthorizedCommand(*identity, command))
		{
			continue;
		}

		if (IsPlayerMoveCommandType(command.typeKey))
		{
			(void)HandleMoveCommand(ctx, command, *input);
			continue;
		}

		if (IsPlayerActionEventCommandType(command.typeKey))
		{
			(void)HandleActionCommand(ctx, command, *input);
			continue;
		}

		if (IsPlayerGuardCommandType(command.typeKey))
		{
			(void)HandleGuardCommand(ctx, command, *input);
		}
	}
}
