#include "pch.h"
#include "LeafNode.h"

NodeStatus ActionNode::OnEvent(EventManager& eventMng)
{
	if (not _future.valid()) {
		_future = eventMng.AddEvent(_eventFunc, _delayMs);
		return NodeStatus::Running;
	}

	if (_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		auto future = _future.get();
		if (auto result = std::get_if<NodeStatus>(&future)) {
			return *result;
		}

		else {
			// NodeStatus가 아닌 다른 Return 값이면 Success 처리
			return NodeStatus::Success;
		}

		{
			// C++ 17 표준 문법
			//
			// 1. auto&&
			//  - Template Auto Type 추론
			//  - Universal Reference (Forwarding Reference) 문법
			// 
			// 2. std::decay_t<>
			//  - Type Normailze
			//  - Reference / const 등을 제거한 순수 타입 Return
			// 
			// 3. decltype()
			//  - Type Return
			//  - std::decay_t<decltype()>의 형태로 많이 사용함
			//
			// 너무 어렵다... 난 직관적인게 더 좋은듯...

			/*return std::visit([](auto&& arg) -> NodeStatus {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, NodeStatus>) {
					return arg;
				}

				else {
					return NodeStatus::Success;
				}
				}, result);*/
		}
	}

	return NodeStatus::Running;
}

void ActionNode::Reset()
{
	// TEMP : 가능하다면 Event자체 취소 기능 구현
	_future = std::future<EventReturn>();
}

NodeStatus ConditionNode::OnEvent(EventManager&)
{
	return _condition() ? NodeStatus::Success : NodeStatus::Failure;
}