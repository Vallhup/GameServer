#pragma once

#include <typeindex>

class IComponent;

class ComponentHost {
public:
	template<typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
		if (GetComponent<T>()) {
			return nullptr;
		}

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
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

	void Update(float deltaTime);

protected:
	std::vector<std::unique_ptr<IComponent>> _components;
	std::unordered_map<std::type_index, IComponent*> _types;
};

