#include "pch.h"
#include "Terrain.h"
#include "DX12Core.h"
#include "VertexIndexBuffer.h"
#include "Importer.h"
#include "Material.h"

void Terrain::Initialize(DX12Core& core, const wstring& basePath, const wstring& heightmapPath, int inGridSize, float inWorldSize, float inHeightScale, float tileSize)
{
	gridSize = inGridSize;
	worldSize = inWorldSize;
	heightScale = inHeightScale;
	tilingSize = tileSize;

	LoadHeightmap(heightmapPath);
	BuildVertices(tileSize);
	BuildIndices();

	vertexIndexBuffer = make_shared<VertexIndexBuffer>();
	vertexIndexBuffer->Initialize(
		core.GetDevice(),
		core.GetGraphicsCmdList(),
		vertices,
		indices
	);

	wstring texBasePath = basePath.substr(0, basePath.find_last_of(L"/\\") + 1);

	Importer importer;

	if (importer.LoadMaterialOnly(basePath)) {
		const auto& mats = importer.GetMaterials();

		material = make_shared<Material>();
		material->LoadFromMaterialData(core.GetDevice(), core.GetGraphicsCmdList(), mats[0], texBasePath);
	}
	else
	{
		MaterialData matData = {};
		string pathStr(basePath.begin(), basePath.end());
		matData.baseColorTexPath = pathStr;

		material = make_shared<Material>();
		material->LoadFromMaterialData(core.GetDevice(), core.GetGraphicsCmdList(), matData);
	}

	objectCB = make_unique<UploadBuffer>();
	objectCB->Initialize(core.GetDevice(), CONSTANT_BUFFER_ALIGNMENT);

	int useTexture = material ? 1 : 0;
	UINT matIndex = material ? material->GetMaterialIndex() : 0;

	ObjectConstants obj = {};
	obj.world = XMMatrixTranspose(XMMatrixIdentity());
	obj.useTexture = useTexture;
	obj.useInstancing = 0;
	obj.materialIndex = matIndex;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);

	char buf[256];
	sprintf_s(buf, "Terrain created: gridSize=%d, worldSize=%.1f, vertices=%zu, indices=%zu\n",
		gridSize, worldSize, vertices.size(), indices.size());
	OutputDebugStringA(buf);
}

void Terrain::Render(ID3D12GraphicsCommandList* cmdList)
{
	if (vertexIndexBuffer)
	{
		vertexIndexBuffer->Bind(cmdList);
		vertexIndexBuffer->Draw(cmdList);
	}
}

void Terrain::SetPosition(float x, float y, float z)
{
	position = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rotation.x),
		XMConvertToRadians(rotation.y),
		XMConvertToRadians(rotation.z)
	);
	XMMATRIX transMat = XMMatrixTranslation(x, y, z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * rotMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = material ? 1 : 0;
	obj.useInstancing = 0;
	obj.materialIndex = material ? material->GetMaterialIndex() : 0;
	obj.useTerrainBlend = (splatmap1Index != 0xFFFFFFFF) ? 2 : 0;
	obj.splatmap1Index  = splatmap1Index;
	obj.splatmap2Index  = splatmap2Index;
	obj.splatUVScale    = splatUVScale;
	obj.splatLayerCount = splatLayerCount;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Terrain::SetRotation(float x, float y, float z)
{
	rotation = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(x),
		XMConvertToRadians(y),
		XMConvertToRadians(z)
	);
	XMMATRIX transMat = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * rotMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = material ? 1 : 0;
	obj.useInstancing = 0;
	obj.materialIndex = material ? material->GetMaterialIndex() : 0;
	obj.useTerrainBlend = (splatmap1Index != 0xFFFFFFFF) ? 2 : 0;
	obj.splatmap1Index  = splatmap1Index;
	obj.splatmap2Index  = splatmap2Index;
	obj.splatUVScale    = splatUVScale;
	obj.splatLayerCount = splatLayerCount;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Terrain::SetScale(float x, float y, float z)
{
	scale = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(x, y, z);
	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rotation.x),
		XMConvertToRadians(rotation.y),
		XMConvertToRadians(rotation.z)
	);
	XMMATRIX transMat = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * rotMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = material ? 1 : 0;
	obj.useInstancing = 0;
	obj.materialIndex = material ? material->GetMaterialIndex() : 0;
	obj.useTerrainBlend = (splatmap1Index != 0xFFFFFFFF) ? 2 : 0;
	obj.splatmap1Index  = splatmap1Index;
	obj.splatmap2Index  = splatmap2Index;
	obj.splatUVScale    = splatUVScale;
	obj.splatLayerCount = splatLayerCount;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Terrain::LoadSplatmap(DX12Core& core, const wstring& binPath, const wstring& texBasePath)
{
	ifstream file(binPath, ios::binary);
	if (!file.is_open())
	{
		OutputDebugStringA("Failed to open splatmap BIN\n");
		return;
	}

	uint32_t W, H, C;
	file.read(reinterpret_cast<char*>(&W), sizeof(W));
	file.read(reinterpret_cast<char*>(&H), sizeof(H));
	file.read(reinterpret_cast<char*>(&C), sizeof(C));

	const int layerCount = static_cast<int>(C);
	if (layerCount == 0) return;

	vector<string> texNames(layerCount);
	for (int i = 0; i < layerCount; ++i)
	{
		uint16_t len;
		file.read(reinterpret_cast<char*>(&len), sizeof(len));
		texNames[i].resize(len);
		file.read(texNames[i].data(), len);
	}

	MaterialData matData = {};
	string* matPtrs[] = {
		&matData.baseColorTexPath, &matData.normalTexPath,
		&matData.roughnessTexPath, &matData.metallicTexPath,
		&matData.heightTexPath,    &matData.alphaTexPath,
		&matData.emissionTexPath,  &matData.aoTexPath
	};
	for (int i = 0; i < layerCount && i < 8; ++i)
		*matPtrs[i] = texNames[i] + ".dds";

	material->LoadFromMaterialData(core.GetDevice(), core.GetGraphicsCmdList(), matData, texBasePath);

	const size_t cellCount = static_cast<size_t>(W) * H;
	vector<uint8_t> raw(cellCount * C);
	file.read(reinterpret_cast<char*>(raw.data()), raw.size());

	const int numSplatTex = (layerCount + 3) / 4; // 최대 2장
	vector<uint8_t> tex0(cellCount * 4, 0);
	vector<uint8_t> tex1(cellCount * 4, 0);

	for (size_t idx = 0; idx < cellCount; ++idx)
	{
		for (int i = 0; i < layerCount && i < 4; ++i)
			tex0[idx * 4 + i] = raw[idx * C + i];
		for (int i = 4; i < layerCount && i < 8; ++i)
			tex1[idx * 4 + (i - 4)] = raw[idx * C + i];
	}

	splatmap1Index = Material::RegisterTextureFromMemory(
		core.GetDevice(), core.GetGraphicsCmdList(),
		tex0.data(), static_cast<int>(W), static_cast<int>(H), DXGI_FORMAT_R8G8B8A8_UNORM);

	if (numSplatTex > 1)
		splatmap2Index = Material::RegisterTextureFromMemory(
			core.GetDevice(), core.GetGraphicsCmdList(),
			tex1.data(), static_cast<int>(W), static_cast<int>(H), DXGI_FORMAT_R8G8B8A8_UNORM);

	splatUVScale = tilingSize / worldSize;
	splatLayerCount = layerCount;

	ObjectConstants obj = {};
	obj.world           = XMMatrixTranspose(XMMatrixIdentity());
	obj.useTexture      = 1;
	obj.useInstancing   = 0;
	obj.materialIndex   = material->GetMaterialIndex();
	obj.useTerrainBlend = 2;
	obj.splatmap1Index  = splatmap1Index;
	obj.splatmap2Index  = splatmap2Index;
	obj.splatUVScale    = splatUVScale;
	obj.splatLayerCount = splatLayerCount;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);

	OutputDebugStringA("Splatmap (binary) loaded and registered\n");
}

void Terrain::LoadHeightmap(const wstring& path)
{
	ifstream file(path, ios::binary);
	if (!file.is_open())
	{
		OutputDebugStringA("Failed to load heightmap for Terrain\n");
		return;
	}

	file.seekg(0, ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, ios::beg);

	size_t pixelCount = fileSize / sizeof(unsigned short);
	int dimension = static_cast<int>(sqrt(pixelCount));

	heightmapWidth = dimension;
	heightmapHeight = dimension;

	vector<unsigned short> rawData(pixelCount);
	file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
	file.close();

	heightmapData.resize(pixelCount);
	for (size_t i = 0; i < pixelCount; ++i)
	{
		heightmapData[i] = rawData[i] / 65535.0f;
	}

	char buf[128];
	sprintf_s(buf, "Heightmap loaded: %dx%d\n", heightmapWidth, heightmapHeight);
	OutputDebugStringA(buf);
}

void Terrain::BuildVertices(float tileSize)
{
	vertices.clear();
	vertices.reserve((gridSize + 1) * (gridSize + 1));

	for (int z = 0; z <= gridSize; ++z)
	{
		for (int x = 0; x <= gridSize; ++x)
		{
			Vertex vertex = {};

			float px = (float)x / gridSize * worldSize;
			float pz = (float)z / gridSize * worldSize;

			float normalizedU = (float)x / gridSize;
			float normalizedV = (float)z / gridSize;

			float tilingSize = tileSize;
			float uvScale = worldSize / tilingSize;
			float u = normalizedU * uvScale;
			float v = normalizedV * uvScale;

			float height = 0.0f;
			if (!heightmapData.empty())
			{
				float hx = normalizedU * (heightmapWidth - 1);
				float hz = normalizedV * (heightmapHeight - 1);

				int x0 = static_cast<int>(floor(hx));
				int z0 = static_cast<int>(floor(hz));
				int x1 = min(x0 + 1, heightmapWidth - 1);
				int z1 = min(z0 + 1, heightmapHeight - 1);

				float fx = hx - x0;
				float fz = hz - z0;

				float h00 = heightmapData[z0 * heightmapWidth + x0];
				float h10 = heightmapData[z0 * heightmapWidth + x1];
				float h01 = heightmapData[z1 * heightmapWidth + x0];
				float h11 = heightmapData[z1 * heightmapWidth + x1];

				float h0 = h00 * (1.0f - fx) + h10 * fx;
				float h1 = h01 * (1.0f - fx) + h11 * fx;
				height = h0 * (1.0f - fz) + h1 * fz;
			}

			vertex.pos = XMFLOAT3(px, height * heightScale, pz);
			vertex.uv = XMFLOAT2(u, v);

			vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			vertex.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);

			vertex.weights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			vertex.indices = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			vertex.color = XMFLOAT4(0.5059f, 0.7569f, 0.2784f, 1.0f);  

			vertices.push_back(vertex);
		}
	}

	for (int z = 0; z <= gridSize; ++z)
	{
		for (int x = 0; x <= gridSize; ++x)
		{
			int idx = z * (gridSize + 1) + x;

			float hL = (x > 0) ? vertices[idx - 1].pos.y : vertices[idx].pos.y;
			float hR = (x < gridSize) ? vertices[idx + 1].pos.y : vertices[idx].pos.y;
			float hD = (z > 0) ? vertices[idx - (gridSize + 1)].pos.y : vertices[idx].pos.y;
			float hU = (z < gridSize) ? vertices[idx + (gridSize + 1)].pos.y : vertices[idx].pos.y;

			float cellSize = worldSize / gridSize;
			XMVECTOR normal = XMVector3Normalize(XMVectorSet(
				(hL - hR) / (2.0f * cellSize),
				1.0f,
				(hD - hU) / (2.0f * cellSize),
				0.0f
			));

			XMStoreFloat3(&vertices[idx].normal, normal);

			vertices[idx].tangent = XMFLOAT3(1.0f, (hR - hL) / (2.0f * cellSize), 0.0f);
		}
	}
}

void Terrain::BuildIndices()
{
	indices.clear();
	indices.reserve(gridSize * gridSize * 6);

	for (int z = 0; z < gridSize; ++z)
	{
		for (int x = 0; x < gridSize; ++x)
		{
			int topLeft = z * (gridSize + 1) + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * (gridSize + 1) + x;
			int bottomRight = bottomLeft + 1;

			indices.push_back(topRight);
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}
}

float Terrain::SampleHeightAt(float worldX, float worldZ) const
{
	if (heightmapData.empty() || worldSize <= 0.0f) return 0.0f;

	float u = worldX / worldSize;
	float v = worldZ / worldSize;

	if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
	{
		return 0.0f;
	}

	float hx = u * (heightmapWidth - 1);
	float hz = v * (heightmapHeight - 1);

	int x0 = static_cast<int>(floor(hx));
	int z0 = static_cast<int>(floor(hz));
	int x1 = min(x0 + 1, heightmapWidth - 1);
	int z1 = min(z0 + 1, heightmapHeight - 1);

	float fx = hx - x0;
	float fz = hz - z0;

	float h00 = heightmapData[z0 * heightmapWidth + x0];
	float h10 = heightmapData[z0 * heightmapWidth + x1];
	float h01 = heightmapData[z1 * heightmapWidth + x0];
	float h11 = heightmapData[z1 * heightmapWidth + x1];

	float h0 = h00 * (1.0f - fx) + h10 * fx;
	float h1 = h01 * (1.0f - fx) + h11 * fx;
	float height = h0 * (1.0f - fz) + h1 * fz;

	return height * heightScale;
}
