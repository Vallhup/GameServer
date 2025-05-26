#include "pch.h"
#include "Importer.h"
#include "VertexIndexBuffer.h"
#include <fstream>

bool Importer::Load(const std::wstring& binPath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
	ifstream ifs{ binPath, ios::binary };
    if (!ifs)
        return false;

    struct header {
        uint32_t verticessize;
        uint32_t indicessize;
    };

    header h;

    ifs.read((char*)&h, sizeof(header));

    outVertices.resize(h.verticessize);
    outIndices.resize(h.indicessize);

    ifs.read((char*)outVertices.data(), sizeof(Vertex)* outVertices.size());
    ifs.read((char*)outIndices.data(), sizeof(UINT)* outIndices.size());

    return true;
}

void Importer::LoadPos(const std::wstring& binPath, std::vector<XMFLOAT3>& outPositions)
{
    ifstream ifs{ binPath, ios::binary };
    if (!ifs)
        return;

    uint32_t size;
    ifs.read((char*)&size, sizeof(uint32_t));
    outPositions.resize(size);
    ifs.read((char*)outPositions.data(), sizeof(XMFLOAT3) * size);
}

void Importer::LoadNormal(const std::wstring& binPath, std::vector<XMFLOAT3>& outNormals)
{
    ifstream ifs{ binPath, ios::binary };
    if (!ifs)
        return;

    uint32_t size;
    ifs.read((char*)&size, sizeof(uint32_t));
    outNormals.resize(size);
    ifs.read((char*)outNormals.data(), sizeof(XMFLOAT3) * size);
}

void Importer::LoadUV(const std::wstring& binPath, std::vector<XMFLOAT2>& outUVs)
{
    ifstream ifs{ binPath, ios::binary };
    if (!ifs)
        return;

    uint32_t size;
    ifs.read((char*)&size, sizeof(uint32_t));
    outUVs.resize(size);
    ifs.read((char*)outUVs.data(), sizeof(XMFLOAT2) * size);
}

void Importer::LoadIndices(const std::wstring& binPath, std::vector<UINT>& outIndices)
{
    ifstream ifs{ binPath, ios::binary };
    if (!ifs)
        return;

    uint32_t size;
    ifs.read((char*)&size, sizeof(uint32_t));
    outIndices.resize(size);
    ifs.read((char*)outIndices.data(), sizeof(UINT) * size);
}

bool Importer::LoadSeparated(const std::wstring& basePath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    std::vector<XMFLOAT3> positions, normals;
    std::vector<XMFLOAT2> uvs;

    // 각 파일 로드
    LoadPos(basePath + L"_pos.bin", positions);
    LoadNormal(basePath + L"_norm.bin", normals);
    LoadUV(basePath + L"_uv.bin", uvs);
    LoadIndices(basePath + L"_idx.bin", outIndices);

    // Vertex 구조체로 합치기
    outVertices.resize(positions.size());

    for (size_t i = 0; i < positions.size(); ++i) {
        outVertices[i].position = positions[i];
        outVertices[i].normal = normals[i];
        outVertices[i].uv = uvs[i];
    }

    return true;
}