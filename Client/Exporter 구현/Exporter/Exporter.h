#pragma once
#include <string>
#include <vector>

struct Vertex;

struct MeshBinHeader
{
	uint32_t vertexCount;
	uint32_t indexCount;
};

class Exporter
{
public:
	bool ExportToBinary(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	bool ExportToText(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	bool ExportPosition(const std::wstring& path, const std::vector<Vertex>& vertices);
	bool ExportNormal(const std::wstring& path, const std::vector<Vertex>& vertices);
	bool ExportUV(const std::wstring& path, const std::vector<Vertex>& vertices);

	bool ExportIndices(const std::wstring& path, const std::vector<uint32_t>& indices);
};
