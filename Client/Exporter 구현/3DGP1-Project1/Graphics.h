#pragma once

class GameObject;

struct Viewport
{
	float Left;
	float Top;
	float Width;
	float Height;
};

class Graphics
{
public:
	static Graphics& Get();

	void SetProjection(XMFLOAT4X4& projection ,const float fovdegrees, const float nearz, const float farz);
	void SetProjection(XMFLOAT4X4& projection, const float width, const float height, const float nearz, const float farz);

	void SetView(XMFLOAT4X4& view, const XMFLOAT3& pos, const XMFLOAT3& lookdir);

	void DrawMesh(const XMFLOAT4X4& view, const XMFLOAT4X4& projection,
		const Viewport& viewport, const XMMATRIX& world, const GameObject* obj, UINT objIndex) const;

	void DrawObjectsToWorld(const GameObject* obj, const XMFLOAT4X4& view, const XMFLOAT4X4& projection,
		const Viewport& viewport, const XMMATRIX& parentWorld, UINT& currentIndex) const;

public:
	XMVECTOR ChangeScreenPointToRay(const XMFLOAT4X4& view, const XMFLOAT4X4& projection,
									const Viewport& viewport, const XMFLOAT2 screenPoint) const;

	float GetAngleBetweenNormals(const XMVECTOR& v0, const XMVECTOR& v1);

private:
	static constexpr int MAX_INDEX = { 62 };
};
