#pragma once

class Mesh
{
public:
	Mesh() = default;
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	~Mesh() = default;

public:
	void Initialize(const XMFLOAT3* Vert, const size_t vCount, const UINT* Idx, const size_t IdxCount);
	void Release();

	const XMFLOAT3* GetVertices() const;
	const UINT* GetIndices() const;

	size_t GetVertCount() const;
	size_t GetIdxCount() const;

private:
	unique_ptr<XMFLOAT3[]> mVertices = nullptr;
	size_t mVertCount = 0;

	unique_ptr<UINT[]> mIndices = nullptr;
	size_t mIndexCount = 0;
};