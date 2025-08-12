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
			return std::hash<int>{}(id.value) ^ (std::hash<int>{}(id.type) << 1);
		}
	};
}

class GameObject : public ComponentHost {
public:
	GameObject() = delete;
	GameObject(ObjectId id) : _id(id) {}
	virtual ~GameObject() = default;

public:
	const ObjectId& GetId() const { return _id; }

protected:
	ObjectId _id;
};