#pragma once

class CompositeNode : public BTNode {
public:
	CompositeNode() = delete;
	CompositeNode(int id) : BTNode(id), _currentIndex(0) {}
	virtual ~CompositeNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) = 0;
	virtual void Reset() override
	{
		for (auto& child : _children) {
			child->Reset();
		}
	}

public:
	void AddChild(const std::shared_ptr<BTNode>& child) { _children.push_back(child); }

protected:
	std::vector<std::shared_ptr<BTNode>> _children;
	size_t _currentIndex;
};

class SequenceNode : public CompositeNode {
public:
	SequenceNode() = delete;
	SequenceNode(int id) : CompositeNode(id) {}
	virtual ~SequenceNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};

class SelectorNode : public CompositeNode {
public:
	SelectorNode() = delete;
	SelectorNode(int id) : CompositeNode(id) {}
	virtual ~SelectorNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};

class ParallelNode : public CompositeNode {
public:
	ParallelNode() = delete;
	ParallelNode(int id) : CompositeNode(id) {}
	virtual ~ParallelNode() = default;

public:
	virtual NodeStatus OnEvent(EventManager& eventMng) override;
};