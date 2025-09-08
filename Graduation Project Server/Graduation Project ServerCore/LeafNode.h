#pragma once

class LeafNode : public BTNode {
public:
	LeafNode() = delete;
	LeafNode(int id) : BTNode(id) {}
	virtual ~LeafNode() = default;

public:
	virtual NodeStatus OnEvent() = 0;
	virtual void Reset() = 0;
};

class ActionNode : public LeafNode {
	using EventFunc = std::function<NodeStatus()>;
public:
	ActionNode() = delete;
	ActionNode(int id, const EventFunc& eventFunc, float delayMs)
		: LeafNode(id), _eventFunc(eventFunc), _delayMs(delayMs) {}
	virtual ~ActionNode() = default;

public:
	virtual NodeStatus OnEvent() override;
	virtual void Reset() override;

private:
	EventFunc _eventFunc;
	float _delayMs;
	std::future<NodeStatus> _future;
};

class ConditionNode : public LeafNode {
	using ConditionFunc = std::function<bool()>;

public:
	ConditionNode() = delete;
	ConditionNode(int id, const ConditionFunc& condition)
		: LeafNode(id), _condition(condition) {}
	virtual ~ConditionNode() = default;

public:
	virtual NodeStatus OnEvent() override;
	virtual void Reset() override {}

private:
	ConditionFunc _condition;
};