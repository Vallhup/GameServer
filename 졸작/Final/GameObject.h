#pragma once

class Component;

class GameObject
{
public:
	template<typename T>
	T* AddComponent();

	template<typename T>
	T* GetComponent();

	virtual void Update(float deltaTime);

public:
	// Server Test
	int GetId() const { return _id; }
	void SetId(int id) { _id = id; }

private:
	vector<unique_ptr<Component>> components;

protected:
	// Server Test
	int _id;
};

template<typename T>
inline T* GameObject::AddComponent()
{
	if (GetComponent<T>())
		return nullptr;

	auto component = make_unique<T>();
	T* rawPtr = component.get();
	component->owner = this;
	components.push_back(std::move(component));

	return rawPtr;
}

template<typename T>
inline T* GameObject::GetComponent()
{
	for (auto& comp : components)
	{
		if (T* casted = dynamic_cast<T*>(comp.get()))
			return casted;
	}

	return nullptr;
}
