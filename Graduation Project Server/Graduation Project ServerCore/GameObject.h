#pragma once

enum ObjectType : char { Static, Dynamic };

struct ObjectId {
	int value;
	ObjectType type;

	ObjectId() = delete;
	ObjectId(int v, ObjectType t) : value(v), type(t) {}

	bool operator==(const ObjectId& other) const { return value == other.value; }
	bool operator!=(const ObjectId& other) const { return value != other.value; }
};

namespace std {
	template<>
	struct std::hash<ObjectId> {
		size_t operator()(const ObjectId& id) const {
			return std::hash<int>{}(id.value);
		}
	};
}

class GameObject {
public:
	GameObject() = delete;
	GameObject(ObjectId id, vec3 pos) : _id(id), _pos(pos) {}
	virtual ~GameObject() = default;

public:
	const ObjectId& GetId() const { return _id; }
	vec3 GetPosition() const { return _pos; }
	std::span<const std::unique_ptr<CollisionShape>> GetCollisionShapes() const { return { _collisionShapes.data(), _collisionShapes.size() }; }

protected:
	ObjectId _id;
	vec3 _pos;
	std::vector<std::unique_ptr<CollisionShape>> _collisionShapes;
};

class StaticGameObject : public GameObject {
public :
	StaticGameObject() = delete;
	StaticGameObject(ObjectId id, vec3 pos) : GameObject(id, pos) {}
	virtual ~StaticGameObject() = default;

public:
	// 상호작용 함수 (어떤식으로 구현할지 고민 중)
	// 1. StaticGameObject에서 Pure Virtual Function 구현
	// 2. CallBack or Lambda Function 등록 (Script 연동할 때 많이 씀)
	// 3. State Pattern

	// virtual void Interact(const std::shared_ptr<DynamicGameObject>& actor) = 0;

protected:

};

class DynamicGameObject : public GameObject {
public:
	DynamicGameObject() = delete;
	DynamicGameObject(ObjectId id, vec3 pos) : GameObject(id, pos) {}
	virtual ~DynamicGameObject() = default;

public:
	virtual void Update(float deltaTime) = 0;
	virtual void Move(float deltaTime) = 0;
	virtual void TakeDamage(int damage) = 0;
	virtual void Die() = 0;
	virtual void Revive() = 0;

protected:

};