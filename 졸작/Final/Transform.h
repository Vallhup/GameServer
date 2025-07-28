#pragma once

class Transform
{
public:
	XMMATRIX CreateWorldMatrix() const;
	XMMATRIX CreateBasisMatrix() const;

	const XMFLOAT3& GetPosition() const;
	const XMVECTOR GetPositionVec() const;

	const XMFLOAT3& GetRotation() const;
	const XMVECTOR GetRotationVec() const;

	const XMFLOAT3& GetScale() const;

	const XMFLOAT3& GetLookDir() const;
	const XMVECTOR GetLookVec() const;

	void SetPosition(const XMFLOAT3& pos);
	void SetPositionVec(const XMVECTOR& pos);	
												 
	void SetRotation(const XMFLOAT3& rot);
	void SetRotationVec(const XMVECTOR& rot);
	void UpdateRotationBasis();

	void SetScale(const XMFLOAT3& scale);

private:
	XMFLOAT3 mPosition = {};
	XMFLOAT3 mRotation = {};
	XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };

	XMFLOAT3 mRightDirection = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUpDirection = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLookDirection = { 0.0f, 0.0f, 1.0f };
};
