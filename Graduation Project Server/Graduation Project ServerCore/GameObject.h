#pragma once

#include <typeindex>

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
	struct hash<ObjectId> {
		using UnderType = std::underlying_type_t<ObjectType>;
		size_t operator()(const ObjectId& id) const {
			return std::hash<int>{}(id.value) ^ 
				(std::hash<UnderType>{}(static_cast<UnderType>(id.type)) << 1);
		}
	};
}

class GameObject {
public:
	GameObject() = delete;
	GameObject(ObjectId id, Instance& instance) : _id(id), _instance(instance) {}
	virtual ~GameObject();

public:
	template<typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
		if (GetComponent<T>()) {
			return nullptr;
		}

		auto component = std::make_unique<T>(*this, _instance, std::forward<Args>(args)...);
		T* raw = component.get();

		_types[std::type_index(typeid(T))] = raw;
		_components.push_back(std::move(component));
		raw->Register();

		return raw;
	}

	template<typename T>
	bool RemoveComponent()
	{
		if (not Getcomponent<T>()) {
			return false;
		}

		auto it = _types.find(std::type_index(typeid(T)));
		if (it != _types.end()) {
			auto* component = it->second;

			for (auto& uniqeCmp : _components) {
				if (component == uniqeCmp.get()) {
					component->Deregister();

					_components.erase(uniqeCmp);
					_types.erase(it);

					return true;
				}
			}
		}

		return false;
	}

	template<typename T>
	T* GetComponent() const
	{
		auto it = _types.find(std::type_index(typeid(T)));
		if (it != _types.end()) {
			return static_cast<T*>(it->second);
		}

		return nullptr;
	}

public:
	const ObjectId& GetId() const { return _id; }

protected:
	ObjectId _id;
	Instance& _instance;

	std::vector<std::unique_ptr<IComponent>> _components;
	std::unordered_map<std::type_index, IComponent*> _types;
};