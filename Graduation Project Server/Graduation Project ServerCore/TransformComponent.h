#pragma once

class TransformComponent : public IComponent {
public:
	TransformComponent() = delete;
	TransformComponent(GameObject& owner, Instance* instance, const vec3& pos) 
		: IComponent(owner, instance), _pos(pos) {}
	virtual ~TransformComponent() = default;

public:
	virtual void NetworkUpdate() override;

public:
	const vec3& GetPosition() const { return _pos; }

	void SetPosition(const vec3& pos) { _pos = pos; ++_version; }
	void Translate(const vec3& delta) { _pos += delta; ++_version; }

private:
	vec3 _pos;
};

