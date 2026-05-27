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

	template<typename T>
	const T* GetComponent() const;

	auto& GetComponents() { return components; }

	virtual void Update(float deltaTime);
	void RenderDebugBoundingBox(DX12Core& core, const XMFLOAT4& color);

public:
	int GetId() const { return mId; }
	bool IsStatic() const { return isStatic; }

	void SetId(int id) { mId = id; }
	void SetStatic(bool value) { isStatic = value; }
	void SetDistanceCull(bool cull, float distance) { needDistanceCull = cull; cullDistance = distance; }
	bool NeedDistanceCull() const { return needDistanceCull; }
	float GetCullDistance() const { return cullDistance; }

	void SetObstructsCamera(bool value) { obstructsCamera = value; }
	bool ObstructsCamera() const { return obstructsCamera; }

	const BoundingBox& GetLocalBoundingBox() const { return localBoundingBox; }
	const BoundingOrientedBox& GetWorldBoundingBox() const { return worldBoundingBox; }
	void SetLocalBoundingBox(const BoundingBox& box) { localBoundingBox = box; }
	void SetWorldBoundingBox(const BoundingOrientedBox& box) { worldBoundingBox = box; }

	bool IsVisible(const BoundingFrustum& frustum, const XMVECTOR& camPos) const;

private:
	bool IsInFrustum(const BoundingFrustum& frustum) const;
	bool IsInRange(const XMVECTOR& camPos) const;

private:
	unordered_map<type_index, unique_ptr<Component>> components;
	
	BoundingBox localBoundingBox;
	BoundingOrientedBox worldBoundingBox;

	ComPtr<ID3D12Resource> debugLineBuffer;

	int mId;
	bool isStatic = false;
	bool obstructsCamera = true;

	bool needDistanceCull;
	float cullDistance;
};

template<typename T>
inline T* GameObject::AddComponent()
{
	auto component = make_unique<T>();
	T* rawPtr = component.get();
	component->owner = this;
	component->Init();
	components[typeid(T)] = move(component);

	return rawPtr;
}

template<typename T>
inline T* GameObject::GetComponent()
{
	auto it = components.find(typeid(T));
	if (it == components.end()) return nullptr;
	return static_cast<T*>(it->second.get());
}

template<typename T>
inline const T* GameObject::GetComponent() const
{
	auto it = components.find(typeid(T));
	if (it == components.end()) return nullptr;
	return static_cast<const T*>(it->second.get());
}
