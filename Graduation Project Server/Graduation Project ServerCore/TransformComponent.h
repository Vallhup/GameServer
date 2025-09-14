#pragma once

class TransformComponent : public IComponent {
public:
	TransformComponent() = delete;
	TransformComponent(GameObject& owner, Instance* instance, const vec3& pos) 
		: IComponent(owner, instance), _pos(pos), _angle(0.0f) {}
	virtual ~TransformComponent() = default;

public:
	virtual void NetworkUpdate() override;

public:
	const vec3& GetPosition() const { return _pos; }
	float GetAngle() const { return _angle; }

	void SetPosition(const vec3& pos) { _pos = pos; ++_version; }
	void Translate(const vec3& delta, bool isMoving = true);

private:
	vec3 _pos;
	float _angle;
};

