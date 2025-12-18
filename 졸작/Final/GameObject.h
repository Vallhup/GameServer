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
	
	const BoundingBox& GetLocalBoundingBox() const { return localBoundingBox; }
	const BoundingBox& GetWorldBoundingBox() const { return worldBoundingBox; }
	void SetLocalBoundingBox(const BoundingBox& box) { localBoundingBox = box; }
	void SetWorldBoundingBox(const BoundingBox& box) { worldBoundingBox = box; }

	// Debugging Code
	/*void DebugBoundingBox(const string& objName) const {
		OutputDebugStringA(("=== " + objName + " BoundingBox ===\n").c_str());

		OutputDebugStringA(("Local  Center: (" +
			to_string(localBoundingBox.Center.x) + ", " +
			to_string(localBoundingBox.Center.y) + ", " +
			to_string(localBoundingBox.Center.z) + ")\n").c_str());

		OutputDebugStringA(("Local  Extents: (" +
			to_string(localBoundingBox.Extents.x) + ", " +
			to_string(localBoundingBox.Extents.y) + ", " +
			to_string(localBoundingBox.Extents.z) + ")\n").c_str());

		OutputDebugStringA(("World  Center: (" +
			to_string(worldBoundingBox.Center.x) + ", " +
			to_string(worldBoundingBox.Center.y) + ", " +
			to_string(worldBoundingBox.Center.z) + ")\n").c_str());

		OutputDebugStringA(("World  Extents: (" +
			to_string(worldBoundingBox.Extents.x) + ", " +
			to_string(worldBoundingBox.Extents.y) + ", " +
			to_string(worldBoundingBox.Extents.z) + ")\n").c_str());
	}*/

private:
	vector<unique_ptr<Component>> components;
	
	BoundingBox localBoundingBox;
	BoundingBox worldBoundingBox;

protected:
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
