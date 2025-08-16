#pragma once

enum class NodeStatus : char {
	Success,
	Failure,
	Running
};

class BTNode {
public:
	BTNode() = delete;
	BTNode(int id) : _id(id) {}
	virtual ~BTNode() = default;

public:
	virtual NodeStatus OnEvent(class EventManager& eventMng) = 0;
	virtual void Reset() = 0;

public:
	int GetId() const { return _id; }

protected:
	int _id;
};

class RootNode : public BTNode {
public:
	RootNode() = delete;
	RootNode(const std::shared_ptr<BTNode> child) : BTNode(0), _child(child) {}
	virtual ~RootNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override { return _child->OnEvent(eventMng); }
	virtual void Reset() override { _child->Reset(); }

private:
	std::shared_ptr<BTNode> _child;
};