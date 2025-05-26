#pragma once

struct Vertex;

class Importer
{
public:
	bool Load(const std::wstring& binPath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);
};
