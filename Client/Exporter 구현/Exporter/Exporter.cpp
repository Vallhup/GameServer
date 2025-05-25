#include "Exporter.h"
#include <fstream>
#include "FBXLoader.h"

struct MeshBinHeader
{
    uint32_t vertexCount;
    uint32_t indexCount;
};

bool ExportToBinary(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        return false;

    MeshBinHeader header = {
        static_cast<uint32_t>(vertices.size()),
        static_cast<uint32_t>(indices.size())
    };

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
    ofs.write(reinterpret_cast<const char*>(vertices.data()), sizeof(Vertex) * vertices.size());
    ofs.write(reinterpret_cast<const char*>(indices.data()), sizeof(uint32_t) * indices.size());

    return true;
}

bool ExportToText(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    std::wofstream ofs(path);
    if (!ofs)
        return false;

    ofs << L"# Vertex Count: " << vertices.size() << std::endl;
    ofs << L"# Index Count: " << indices.size() << std::endl;

    ofs << L"\n# Vertices (pos / normal / uv)\n";
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const Vertex& v = vertices[i];
        ofs << i << L": "
            << v.position.x << L" " << v.position.y << L" " << v.position.z << L" | "
            << v.normal.x << L" " << v.normal.y << L" " << v.normal.z << L" | "
            << v.uv.x << L" " << v.uv.y << std::endl;
    }

    ofs << L"\n# Indices (Triangles in 3 indices)\n";

    size_t triangleCount = indices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i)
    {
        size_t base = i * 3;
        ofs << L"[" << i << L"]: "
            << indices[base] << L", "
            << indices[base + 1] << L", "
            << indices[base + 2] << std::endl;
    }

    return true;
}