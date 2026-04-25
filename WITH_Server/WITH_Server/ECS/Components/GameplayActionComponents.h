#pragma once

#include "GameplayComponentPrerequisites.h"

struct ActionInterruptEvent
{
	ActionInterruptCauseType causeType{
		ActionInterruptCauseType::OnHitReceived };
	Entity instigator{ Entity::Null() };
	uint64_t frameIndex{ 0 };
	int priority{ 0 };
};

struct ActionInterruptQueueComp : Component
{
	std::vector<ActionInterruptEvent> events;
};

struct PendingBuffApplyComp : Component
{
	BuffId buffId{ BuffId::None };
};

struct PendingBuffRemoveComp : Component
{
	BuffId buffId{ BuffId::None };
};

enum class LocomotionMode : uint8_t
{
	Idle = 0,
	Walk,
	Run,
	Turn
};

struct ActionStateComp : Component
{
	ActionId actionId{ ActionId::None };
	float elapsedSec{ 0.0f };
	uint32_t actionInstanceId{ 0 };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };

	bool CanIssueAction() const noexcept
	{
		if (actionId == ActionId::None)
		{
			return true;
		}

		const ActionDef* def = FindActionDef(actionId);
		if (nullptr == def)
		{
			return true;
		}

		const float progress =
			(def->duration > 0.0f) ?
			std::clamp(elapsedSec / def->duration, 0.0f, 1.0f) :
			1.0f;

		if (progress >= 1.0f)
		{
			return true;
		}

		for (const ActionCancelRule& cancel : def->transitionRule.cancelRules)
		{
			if (!cancel.aiInterruptible)
			{
				continue;
			}

			if (cancel.windowPolicy == ActionWindowPolicy::Always)
			{
				return true;
			}

			const float windowStart = cancel.windowStartNormalized.value_or(0.0f);
			const float windowEnd = cancel.windowEndNormalized.value_or(1.0f);
			if (progress >= windowStart && progress <= windowEnd)
			{
				return true;
			}
		}

		return false;
	}
};

struct LocomotionStateComp : Component
{
	LocomotionMode mode{ LocomotionMode::Idle };
	float desiredMoveDirX{ 0.0f };
	float desiredMoveDirZ{ 0.0f };
	float desiredFacingYawRad{ 0.0f };
	float facingYawRad{ 0.0f };
	float currentSpeed{ 0.0f };
	float locomotionAnimPhase01{ 0.0f };
	bool wasLocomotionMoving{ false };
};

struct PendingActionTimelineEvent
{
	EventType eventType{ EventType::PlayEffect };
	float timeNormalized{ 0.0f };
	std::optional<EventPayloadId> payloadId;
	TriggerConditionType conditionType{ TriggerConditionType::Always };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
};

struct ActionTimelineAdvanceComp : Component
{
	ActionId actionId{ ActionId::None };
	uint32_t actionInstanceId{ 0 };
	float prevElapsedSec{ 0.0f };
	float currElapsedSec{ 0.0f };
	bool startedThisFrame{ false };
	std::vector<PendingActionTimelineEvent> events;
};
