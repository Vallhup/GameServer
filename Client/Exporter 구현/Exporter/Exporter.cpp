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

bool Exporter::ExportToBinary(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
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

bool Exporter::ExportToText(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
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

bool Exporter::ExportPosition(const std::wstring& path, const std::vector<Vertex>& vertices)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        return false;

    uint32_t size = static_cast<uint32_t>(vertices.size());
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

    std::vector<XMFLOAT3> positions;
    positions.reserve(size);
    for (const auto& vertex : vertices) {
        positions.push_back(vertex.position);
    }

    ofs.write(reinterpret_cast<const char*>(positions.data()), sizeof(XMFLOAT3) * positions.size());

    return true;
}

bool Exporter::ExportNormal(const std::wstring& path, const std::vector<Vertex>& vertices)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        return false;

    uint32_t size = static_cast<uint32_t>(vertices.size());
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

    std::vector<XMFLOAT3> normals;
    normals.reserve(size);
    for (const auto& vertex : vertices) {
        normals.push_back(vertex.normal);
    }

    ofs.write(reinterpret_cast<const char*>(normals.data()), sizeof(XMFLOAT3) * normals.size());

    return true;
}

bool Exporter::ExportUV(const std::wstring& path, const std::vector<Vertex>& vertices)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        return false;

    uint32_t size = static_cast<uint32_t>(vertices.size());
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

    std::vector<XMFLOAT2> UVs;
    UVs.reserve(size);
    for (const auto& vertex : vertices) {
        UVs.push_back(vertex.uv);
    }

    ofs.write(reinterpret_cast<const char*>(UVs.data()), sizeof(XMFLOAT2) * UVs.size());

    return true;
}

bool Exporter::ExportIndices(const std::wstring& path, const std::vector<uint32_t>& indices)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        return false;

    uint32_t size = indices.size();
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

    ofs.write(reinterpret_cast<const char*>(indices.data()), sizeof(uint32_t) * indices.size());

    return true;
}