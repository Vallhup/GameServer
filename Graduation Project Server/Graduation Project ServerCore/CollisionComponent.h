#pragma once

class CollisionComponent : public IComponent {
public:
	CollisionComponent() = delete;
	CollisionComponent(GameObject& owner, Instance* instance)
		: IComponent(owner, instance) {}
	virtual ~CollisionComponent() = default;

public:
	void Activate(CollisionType type);
	void Deactivate(CollisionType type);

	void ActivateAll();
	void DeactivateAll();

private:
	std::vector<std::unique_ptr<CollisionShape>> _shapes;
};

