#include "pch.h"
#include "Transform.h"

XMMATRIX Transform::CreateWorldMatrix() const
{
	XMVECTOR pos = XMLoadFloat3(&mPosition);
	XMVECTOR scale = XMLoadFloat3(&mScale);

	return XMMatrixScalingFromVector(scale) *
		CreateBasisMatrix() *
		XMMatrixTranslationFromVector(pos);
}

XMMATRIX Transform::CreateBasisMatrix() const
{
	XMVECTOR rot = XMVectorSet(
		XMConvertToRadians(mRotation.x),
		XMConvertToRadians(mRotation.y),
		XMConvertToRadians(mRotation.z),
		0.0f
	);

	return XMMatrixRotationRollPitchYawFromVector(rot);
}

const XMFLOAT3& Transform::GetPosition() const
{
	return mPosition;
}

const XMVECTOR Transform::GetPositionVec() const
{
	return XMLoadFloat3(&mPosition);
}

const XMFLOAT3& Transform::GetRotation() const
{
	return mRotation;
}

const XMVECTOR Transform::GetRotationVec() const
{
	return XMLoadFloat3(&mRotation);
}

const XMFLOAT3& Transform::GetScale() const
{
	return mScale;
}

const XMFLOAT3& Transform::GetLookDir() const
{
	return mLookDirection;
}

const XMVECTOR Transform::GetLookVec() const
{
	return XMLoadFloat3(&mLookDirection);
}

void Transform::SetPosition(const XMFLOAT3& pos)
{
	mPosition = pos;
}

void Transform::SetPositionVec(const XMVECTOR& pos)
{
	XMStoreFloat3(&mPosition, pos);
}

void Transform::SetRotation(const XMFLOAT3& rot)
{
	mRotation = rot;
	UpdateRotationBasis();
}

void Transform::SetRotationVec(const XMVECTOR& rot)
{
	XMStoreFloat3(&mRotation, rot);
	UpdateRotationBasis();
}

void Transform::UpdateRotationBasis()
{
	XMMATRIX basis = CreateBasisMatrix();

	XMVECTOR rightVector = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR upVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	XMStoreFloat3(&mRightDirection, XMVector3Transform(rightVector, basis));
	XMStoreFloat3(&mUpDirection, XMVector3Transform(upVector, basis));
	XMStoreFloat3(&mLookDirection, XMVector3Transform(lookVector, basis));
}

void Transform::SetScale(const XMFLOAT3& scale)
{
	mScale = scale;
}
