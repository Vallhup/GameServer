#include "pch.h"
#include "DecoratorNode.h"

NodeStatus InverterNode::OnEvent(EventManager& eventMng)
{
	NodeStatus status = _child->OnEvent(eventMng);

	switch (status) {
	case NodeStatus::Success: return NodeStatus::Failure;
	case NodeStatus::Failure: return NodeStatus::Success;
	case NodeStatus::Running: return NodeStatus::Running;
	}

	return NodeStatus::Failure;
}

NodeStatus RepeaterNode::OnEvent(EventManager& eventMng)
{
	NodeStatus status = _child->OnEvent(eventMng);

	if (_maxCount == -1) return NodeStatus::Running;
	if (_currentCount++ < _maxCount) return NodeStatus::Running;
	return status;
}

NodeStatus SucceederNode::OnEvent(EventManager& eventMng)
{
	_child->OnEvent(eventMng);
	return NodeStatus::Success;
}

NodeStatus UntilFailNode::OnEvent(EventManager& eventMng)
{
	if (NodeStatus::Failure == _child->OnEvent(eventMng)) {
		return NodeStatus::Success;
	}

	return NodeStatus::Running;
}