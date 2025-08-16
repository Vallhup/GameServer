#pragma once

class DecoratorNode : public BTNode {
public:
	DecoratorNode() = delete;
	DecoratorNode(int id, const std::shared_ptr<BTNode>& child) : BTNode(id), _child(child) {}
	virtual ~DecoratorNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) = 0;
	virtual void Reset() override { _child->Reset(); }

protected:
	std::shared_ptr<BTNode> _child;
};

class InverterNode : public DecoratorNode {
public:
	InverterNode() = delete;
	InverterNode(int id, const std::shared_ptr<BTNode>& child) : DecoratorNode(id, child) {}
	virtual ~InverterNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};

class RepeaterNode : public DecoratorNode {
public:
	RepeaterNode() = delete;
	RepeaterNode(int id, const std::shared_ptr<BTNode>& child, int maxCnt = -1)
		: DecoratorNode(id, child), _maxCount(maxCnt), _currentCount(0) {}
	virtual ~RepeaterNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;

private:
	int _maxCount;
	int _currentCount;
};

class SucceederNode : public DecoratorNode {
public:
	SucceederNode() = delete;
	SucceederNode(int id, const std::shared_ptr<BTNode>& child) : DecoratorNode(id, child) {}
	virtual ~SucceederNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};

class UntilFailNode : public DecoratorNode {
public:
	UntilFailNode() = delete;
	UntilFailNode(int id, const std::shared_ptr<BTNode>& child) : DecoratorNode(id, child) {}
	virtual ~UntilFailNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};