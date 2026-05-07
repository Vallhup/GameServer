#pragma once

#include "Session.h"
#include "SessionFlowTypes.h"

enum class SessionFlowResultCode : uint8_t
{
	Accepted,
	InvalidSession,
	UnknownSession,
	InvalidFlowStage,
	CommandTypeMismatch,
	CloseRequested,
};

struct TransitionResult
{
	bool accepted{ false };
	bool closeRequested{ false };
	SessionStateId nextState{ SessionStateId::Connected };
	SessionCloseReason closeReason{ SessionCloseReason::None };
	SessionFlowResultCode code{ SessionFlowResultCode::InvalidFlowStage };

	static TransitionResult Stay(SessionStateId current) noexcept
	{
		return TransitionResult{
			.accepted = true,
			.nextState = current,
			.code = SessionFlowResultCode::Accepted
		};
	}

	static TransitionResult To(SessionStateId next) noexcept
	{
		return TransitionResult{
			.accepted = true,
			.nextState = next,
			.code = SessionFlowResultCode::Accepted
		};
	}

	static TransitionResult Invalid() noexcept
	{
		return TransitionResult{};
	}

	static TransitionResult Close(SessionCloseReason reason) noexcept
	{
		return TransitionResult{
			.accepted = true,
			.closeRequested = true,
			.nextState = SessionStateId::Closing,
			.closeReason = reason,
			.code = SessionFlowResultCode::CloseRequested
		};
	}
};

struct SessionFlowResult
{
	SessionFlowResultCode code{ SessionFlowResultCode::InvalidSession };
	SessionStateId previousState{ SessionStateId::Connected };
	SessionStateId currentState{ SessionStateId::Connected };
	SessionCloseReason closeReason{ SessionCloseReason::None };

	bool Succeeded() const noexcept
	{
		return code == SessionFlowResultCode::Accepted;
	}
};
