#include "pch.h"
#include "GameObject.h"
#include "VertexIndexBuffer.h"

void GameObject::Release()
{
	GameObject* child = mChild;
	while (child)
	{
		GameObject* next = child->mSibling;
		child->Release();  
		child = next;
	}

	mChild = nullptr;
	mLastChild = nullptr;
	mParent = nullptr;
	mSibling = nullptr;
	mPrevious = nullptr;
}

void GameObject::AddChild(GameObject* child)
{
	if (nullptr != mChild) {
		child->mPrevious = mLastChild;
		mLastChild->mSibling = child;
		child->mSibling = nullptr;
	}
	else {
		child->mParent = this;
		mChild = child;
		child->mSibling = nullptr;
	}
	mLastChild = child;
}

void GameObject::RemoveChild(GameObject* child)
{
	if (not child || not mChild)
        return;

    if (mChild == child)
    {
        mChild = child->mSibling;
        if (mLastChild == child)
            mLastChild = nullptr;
    }
    else
    {
        GameObject* prev = mChild;
        while (prev && prev->mSibling != child)
            prev = prev->mSibling;

        if (prev)
        {
            prev->mSibling = child->mSibling;
            if (mLastChild == child)
                mLastChild = prev;
        }
    }

    child->mParent = nullptr;
    child->mSibling = nullptr;
    child->mPrevious = nullptr;
}

const GameObject* GameObject::GetChild() const
{
	return mChild;
}

const GameObject* GameObject::GetChild(int index)
{
	if (index == 0) return mChild;

	const GameObject* currentChild = mChild;
	int currentIndex = 0;

	while (currentChild && currentIndex < index) {
		currentChild = currentChild->GetSibling();
		currentIndex++;
	}

	return currentChild;
}

int GameObject::GetChildCount() const 
{
	if (not mChild) return 0;

	int count = 1;
	const GameObject* current = mChild;
	while (current->GetSibling()) {
		count++;
		current = current->GetSibling();
	}
	return count;
}

const GameObject* GameObject::GetSibling() const
{
	return mSibling;
}

Transform& GameObject::GetTransform()
{
	return mTransform;
}

const Transform& GameObject::GetTransform() const
{
	return mTransform;
}

VertexIndexBuffer* GameObject::GetMesh() const
{
	return mMesh;
}

bool GameObject::GetActive() const
{
	return activate;
}

bool GameObject::GetTransParent() const
{
	return isTransparent;
}

XMVECTOR GameObject::GetWorldPositionVec() const
{
	XMMATRIX matWorld = XMMatrixIdentity();
	const GameObject* current = this;

	while (current)
	{
		const Transform& tf = current->GetTransform();

		XMVECTOR scale = XMLoadFloat3(&tf.GetScale());
		XMVECTOR rot = XMVectorSet(
			XMConvertToRadians(tf.GetRotation().x),
			XMConvertToRadians(tf.GetRotation().y),
			XMConvertToRadians(tf.GetRotation().z),
			0.0f
		);
		XMVECTOR pos = XMLoadFloat3(&tf.GetPosition());

		XMMATRIX localMat =
			XMMatrixScalingFromVector(scale) *
			XMMatrixRotationRollPitchYawFromVector(rot) *
			XMMatrixTranslationFromVector(pos);

		matWorld = localMat * matWorld;

		current = current->mParent;
	}

	XMVECTOR origin = XMVectorZero();
	return XMVector3TransformCoord(origin, matWorld);
}


XMVECTOR GameObject::GetWorldLookVec() const
{
	XMMATRIX matWorld = XMMatrixIdentity();
	const GameObject* current = this;

	while (current)
	{
		const Transform& tf = current->GetTransform();
		matWorld = XMMatrixMultiply(tf.CreateBasisMatrix(), matWorld);
		current = current->mParent;
	}

	XMVECTOR localLook = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	return XMVector3Normalize(XMVector3TransformNormal(localLook, matWorld));
}

void GameObject::SetMesh(VertexIndexBuffer* mesh)
{
	mMesh = mesh;
}

void GameObject::SetActive(bool active)
{
	activate = active;
}

void GameObject::SetTransparent(bool transparent)
{
	isTransparent = transparent;
}

bool GameObject::IsActive() const
{
	return activate;
}

