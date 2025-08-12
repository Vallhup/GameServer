#pragma once

enum class ObjectType : char { Static, Dynamic };

struct ObjectId {
	int value;
	ObjectType type;

	ObjectId() = delete;
	ObjectId(int v, ObjectType t) : value(v), type(t) {}

	bool operator==(const ObjectId& other) const
	{
		return (value == other.value) and (type == other.type);
	}

	bool operator!=(const ObjectId& other) const 
	{
		return not(*this == other);
	}
};

// std::underlying_type_t
//  - enum or enum class가 내부적으로 사용하는 기본 정수형(underlying type)을 가져오는데 사용

// 추가적으로 Hash함수도 더 어렵게 만들 수 있는데
// 그런건 나중에 해보는걸로...

namespace std {
	template<>
	struct std::hash<ObjectId> {
		using UnderType = std::underlying_type_t<ObjectType>;
		size_t operator()(const ObjectId& id) const {
			return std::hash<int>{}(id.value) ^ 
				(std::hash<UnderType>{}(static_cast<UnderType>(id.type)) << 1);
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