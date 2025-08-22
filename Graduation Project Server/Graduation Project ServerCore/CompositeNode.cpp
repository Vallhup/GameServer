#include "pch.h"
#include "CompositeNode.h"

NodeStatus SequenceNode::OnEvent(EventManager& eventMng)
{
	while (_currentIndex < _children.size()) {
		NodeStatus status = _children[_currentIndex]->OnEvent(eventMng);

		switch (status) {
		case NodeStatus::Success:
			_currentIndex++;
			break;

		case NodeStatus::Failure:
			_currentIndex = 0;
			return  NodeStatus::Failure;

		case NodeStatus::Running:
			return NodeStatus::Running;
		}
	}

	_currentIndex = 0;
	return NodeStatus::Success;
}

NodeStatus SelectorNode::OnEvent(EventManager& eventMng)
{
	while (_currentIndex < _children.size()) {
		NodeStatus status = _children[_currentIndex]->OnEvent(eventMng);

		switch (status) {
		case NodeStatus::Success:
			_currentIndex = 0;
			return NodeStatus::Success;

		case NodeStatus::Failure:
			_currentIndex++;
			break;

		case NodeStatus::Running:
			return NodeStatus::Running;
		}
	}

	_currentIndex = 0;
	return NodeStatus::Failure;
}

NodeStatus ParallelNode::OnEvent(EventManager& eventMng)
{
	// TODO : 자식 노드 동시 실행 (성공, 실패 조건은 마음대로)
	return NodeStatus::Success;
}