#include "pch.h"
#include "Mesh.h"

void Mesh::Initialize(const XMFLOAT3* Vert, const size_t vCount, const UINT* Idx, const size_t IdxCount)
{
	mVertCount = vCount;
	mVertices = make_unique<XMFLOAT3[]>(mVertCount);
	memcpy(mVertices.get(), Vert, sizeof(XMFLOAT3) * mVertCount);

	mIndexCount = IdxCount;
	mIndices = make_unique<UINT[]>(mIndexCount);
	memcpy(mIndices.get(), Idx, sizeof(UINT) * mIndexCount);
}

void Mesh::Release()
{
	mVertices.reset();
	mVertCount = 0;

	mIndices.reset();
	mIndexCount = 0;
}

const XMFLOAT3* Mesh::GetVertices() const
{
	return mVertices.get();
}

const UINT* Mesh::GetIndices() const
{
	return mIndices.get();
}

size_t Mesh::GetVertCount() const
{
	return mVertCount;
}

size_t Mesh::GetIdxCount() const
{
	return mIndexCount;
}
