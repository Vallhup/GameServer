#pragma once
#include "DX12Core.h"

class Component;

class GameObject
{
public:
	template<typename T>
	T* AddComponent();

	template<typename T>
	T* GetComponent();

	virtual void Update(float deltaTime);
	void RenderDebugBoundingBox(DX12Core& core, const XMFLOAT4& color);

public:
	int GetId() const { return mId; }
	bool IsStatic() const { return isStatic; }

	void SetId(int id) { mId = id; }
	void SetStatic(bool value) { isStatic = value; }

	const BoundingBox& GetLocalBoundingBox() const { return localBoundingBox; }
	const BoundingBox& GetWorldBoundingBox() const { return worldBoundingBox; }
	void SetLocalBoundingBox(const BoundingBox& box) { localBoundingBox = box; }
	void SetWorldBoundingBox(const BoundingBox& box) { worldBoundingBox = box; }

	bool IsInFrustum(const BoundingFrustum& frustum) const;

private:
	vector<unique_ptr<Component>> components;
	
	BoundingBox localBoundingBox;
	BoundingBox worldBoundingBox;

	ComPtr<ID3D12Resource> debugLineBuffer;

	int mId;
	bool isStatic = false;
};

template<typename T>
inline T* GameObject::AddComponent()
{
	if (GetComponent<T>())
		return nullptr;

	auto component = make_unique<T>();
	T* rawPtr = component.get();
	component->owner = this;
	component->Init();
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
