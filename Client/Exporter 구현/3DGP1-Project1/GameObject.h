#pragma once
#include "Transform.h"
#include "UploadBuffer.h"

class VertexIndexBuffer;

class GameObject
{
public:
	GameObject() = default;
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	~GameObject() = default;

	void Release();

	void AddChild(GameObject* child);
	void RemoveChild(GameObject* child);

	const GameObject* GetChild() const;
	const GameObject* GetChild(int index);
	int GetChildCount() const;
	const GameObject* GetSibling() const;
	Transform& GetTransform();
	const Transform& GetTransform() const;
	VertexIndexBuffer* GetMesh() const;
	bool GetActive() const;
	bool GetTransParent() const;

	XMVECTOR GetWorldPositionVec() const;
	XMVECTOR GetWorldLookVec() const;

	void SetMesh(VertexIndexBuffer* mesh);
	void SetActive(bool active);
	void SetTransparent(bool transparent);

	bool IsActive() const;

private:
	Transform mTransform = {};
	VertexIndexBuffer* mMesh = nullptr;
	bool activate = true;
	bool isTransparent = false;

private:
	GameObject* mChild = nullptr;
	GameObject* mParent = nullptr;
	GameObject* mSibling = nullptr;
	GameObject* mPrevious = nullptr;
	GameObject* mLastChild = nullptr;
};

