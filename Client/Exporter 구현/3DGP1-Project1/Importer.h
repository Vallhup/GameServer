#pragma once

struct Vertex;

class Importer
{
public:
	bool Load(const std::wstring& binPath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);

	void LoadPos(const std::wstring& binPath, std::vector<XMFLOAT3>& outPositions);
	void LoadNormal(const std::wstring& binPath, std::vector<XMFLOAT3>& outNormals);
	void LoadUV(const std::wstring& binPath, std::vector<XMFLOAT2>& outUVs);
	void LoadIndices(const std::wstring& binPath, std::vector<UINT>& outIndices);

	bool LoadSeparated(const std::wstring& basePath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);
};
