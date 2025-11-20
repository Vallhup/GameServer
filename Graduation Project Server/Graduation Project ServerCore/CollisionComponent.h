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

	const auto& GetShapes() const { return _shapes; }
	void SetShapes(std::vector<std::unique_ptr<CollisionShape>> shapes) { _shapes = std::move(shapes); }

private:
	std::vector<std::unique_ptr<CollisionShape>> _shapes;
};