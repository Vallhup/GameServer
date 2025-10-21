#pragma once

#include <typeindex>
#include <concurrent_vector.h>

#include "Instance.h"

class GameObject {
public:
	GameObject() = delete;
	GameObject(int id, Instance* instance) : _id(id), _instance(instance) {}
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

		return raw;
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

	void LogicUpdate(float deltaTime);
	void NetworkUpdate();

public:
	int GetId() const { return _id; }
	Instance* GetInstance() const { return _instance; }

protected:
	int _id;
	Instance* _instance;

	// 나중에 Character Type 구분 위한 처리 필요
	// Enum or 전용 Component
	// Enum 값으로 생각중

	std::vector<std::unique_ptr<IComponent>> _components;
	std::unordered_map<std::type_index, IComponent*> _types;

	//concurrency::concurrent_vector<std::unique_ptr<IComponent>> _components;
	//concurrency::concurrent_unordered_map<std::type_index, IComponent*> _types;
};