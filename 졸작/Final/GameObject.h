#pragma once

class Component;

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	template<typename T>
	shared_ptr<T> AddComponent();

	template<typename T>
	shared_ptr<T> GetComponent();

	void Update(float deltaTime);

private:
	vector<shared_ptr<Component>> components;
};

template<typename T>
inline shared_ptr<T> GameObject::AddComponent()
{
	if (GetComponent<T>())
		return nullptr;

	auto component = make_shared<T>();
	component->owner = shared_from_this();
	components.push_back(component);

	return component;
}

template<typename T>
inline shared_ptr<T> GameObject::GetComponent()
{
	for (auto& comp : components)
	{
		if (auto casted = dynamic_pointer_cast<T>(comp))
			return casted;
	}

	return nullptr;
}
