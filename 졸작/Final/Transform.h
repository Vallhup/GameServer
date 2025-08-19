#pragma once
#include "Component.h"

class Transform : public Component
{
public:
	void Update(float deltaTime) override;

	void Move(float deltaTime);

	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& pos);

	void SetRotation(float x, float y, float z);
	void SetRotation(const XMFLOAT3& rot);

	void SetScale(float x, float y, float z);
	void SetScale(const XMFLOAT3& scl);

	const XMFLOAT3& GetPosition() const;
	const XMFLOAT3& GetRotation() const;
	const XMFLOAT3& GetScale() const;

	XMMATRIX GetWorldMatrix() const;

private:
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
};

