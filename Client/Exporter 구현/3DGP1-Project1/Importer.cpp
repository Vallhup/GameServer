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
