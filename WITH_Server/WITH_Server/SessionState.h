#pragma once

#include "SessionFlowTypes.h"

class SessionFlowContext;

class ISessionState {
public:
	virtual ~ISessionState() = default;

	virtual SessionStateId Id() const noexcept = 0;
	virtual void OnEnter(SessionFlowContext& ctx) { (void)ctx; }
	virtual void OnExit(SessionFlowContext& ctx) { (void)ctx; }
};
