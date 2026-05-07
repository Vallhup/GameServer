#pragma once

#include <functional>
#include <type_traits>
#include <unordered_map>

#include "SessionFlowCommands.h"
#include "SessionFlowContext.h"
#include "SessionFlowResults.h"

class SessionFlowTransitionTable final {
public:
	using Handler = 
		std::function<TransitionResult(SessionFlowContext&, const ISessionCommand&)>;

	template<typename TCommand, typename THandler>
	void Register(
		SessionStateId stateId,
		SessionCommandId commandId,
		THandler&& handler);

	TransitionResult Dispatch(
		SessionFlowContext& ctx,
		SessionStateId stateId,
		const ISessionCommand& command) const;

	bool Contains(SessionStateId stateId, SessionCommandId commandId) const;
	void Clear() noexcept;

private:
	struct Key
	{
		SessionStateId stateId{ SessionStateId::Connected };
		SessionCommandId commandId{ SessionCommandId::LoginRequested };

		bool operator==(const Key& rhs) const noexcept
		{
			return stateId == rhs.stateId && commandId == rhs.commandId;
		}
	};

	struct KeyHash
	{
		size_t operator()(const Key& key) const noexcept
		{
			return (static_cast<size_t>(key.stateId) << 8) ^
				static_cast<size_t>(key.commandId);
		}
	};

	std::unordered_map<Key, Handler, KeyHash> _transitions;
};

template<typename TCommand, typename THandler>
inline void SessionFlowTransitionTable::Register(
	SessionStateId stateId, 
	SessionCommandId commandId, 
	THandler&& handler)
{
	static_assert(std::is_base_of_v<ISessionCommand, TCommand>);

	_transitions[Key{ stateId, commandId }] =
		[fn = std::forward<THandler>(handler)](
			SessionFlowContext& ctx,
			const ISessionCommand& command) -> TransitionResult
		{
			const auto* typed = dynamic_cast<const TCommand*>(&command);
			if (typed == nullptr)
			{
				TransitionResult result = TransitionResult::Invalid();
				result.code = SessionFlowResultCode::CommandTypeMismatch;
				return result;
			}
			return fn(ctx, *typed);
		};
}