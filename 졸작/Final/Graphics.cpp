#include "pch.h"
#include "Graphics.h"
#include "GameObject.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "VertexIndexBuffer.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "DX12Graphics.h"
#include "DescriptorHeap.h"
#include "Texture.h"

Graphics& Graphics::Get()
{
	static Graphics graphics;
	return graphics;
}

void Graphics::SetProjection(XMFLOAT4X4& projection, const float fovdegrees, const float nearz, const float farz)
{
	XMStoreFloat4x4(&projection, XMMatrixPerspectiveFovLH(XMConvertToRadians(fovdegrees), static_cast<float>(WinSize.x) / WinSize.y, nearz, farz));
}

void Graphics::SetProjection(XMFLOAT4X4& projection, const float width, const float height, const float nearz, const float farz)
{
	XMStoreFloat4x4(&projection, XMMatrixOrthographicLH(width, height, nearz, farz));
}

void Graphics::SetView(XMFLOAT4X4& view, const XMFLOAT3& pos, const XMFLOAT3& lookdir)
{
	XMVECTOR eye = XMLoadFloat3(&pos);
	XMVECTOR at = eye + XMLoadFloat3(&lookdir);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMStoreFloat4x4(&view, XMMatrixLookAtLH(eye, at, up));
}

void Graphics::DrawMesh(const Viewport& viewport, const XMMATRIX& world, const GameObject* obj, UINT objIndex) const
{
	ID3D12GraphicsCommandList* cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
	cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

	if (!obj->GetTransParent())
		cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
	else
		cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetTransparentPSO());

	ID3D12DescriptorHeap* heaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
	
	auto heightTex = GET(DX12Graphics).GetHeightMapTexture();
	cmdList->SetGraphicsRootDescriptorTable(2, heightTex->GetSRV());

	if (obj->IsTextured()) {
		auto groundTex = GET(DX12Graphics).GetGroundTexture();
		cmdList->SetGraphicsRootDescriptorTable(3, groundTex->GetSRV());
	}
	
	obj->GetMesh()->Bind(cmdList);

	ObjectConstants objData;
	objData.world = XMMatrixTranspose(world);
	objData.useTexture = obj->IsTextured() ? 1 : 0;
	
	if (obj->IsTextured())
		objData.heightScale = HEIGHTMAP_SCALE;
	else
		objData.heightScale = 0.0f;

	const UINT constantBufferOffset = objIndex * 256;
	GET(DX12Graphics).GetSceneCB()->CopyData(&objData, sizeof(ObjectConstants), constantBufferOffset);
	cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + constantBufferOffset);
	obj->GetMesh()->Draw(cmdList);
}

void Graphics::DrawObjectsToWorld(const GameObject* obj, const Viewport& viewport, const XMMATRIX& parentWorld, UINT& currentIndex) const
{
	if (!obj) return;

	XMMATRIX world = obj->GetTransform().CreateWorldMatrix() * parentWorld;

	if (obj->GetActive() && obj->GetMesh() != nullptr)
	{
		DrawMesh(viewport, world, obj, currentIndex);
		currentIndex++;
	}
	if (obj->GetSibling())
	{
		DrawObjectsToWorld(obj->GetSibling(), viewport, parentWorld, currentIndex);
	}
	if (obj->GetChild())
	{
		DrawObjectsToWorld(obj->GetChild(), viewport, world, currentIndex);
	}
}

XMVECTOR Graphics::ChangeScreenPointToRay(const XMFLOAT4X4& view, const XMFLOAT4X4& projection, const Viewport& viewport, const XMFLOAT2 screenPoint) const
{
	XMFLOAT3 viewPoint =
	{
		((screenPoint.x - viewport.Left) / (viewport.Width * 0.5f) - 1.0f) / projection._11,
		(-(screenPoint.y - viewport.Top) / (viewport.Height * 0.5f) + 1.0f) / projection._22,
		1.0f,
	};

	XMVECTOR ray = XMLoadFloat3(&viewPoint);
	ray = XMVector3TransformCoord(ray, XMMatrixInverse(nullptr, XMLoadFloat4x4(&view)));

	return ray;
}

float Graphics::GetAngleBetweenNormals(const XMVECTOR& v0, const XMVECTOR& v1)
{
	return XMConvertToDegrees(XMVectorGetX(XMVector3AngleBetweenNormalsEst(v0, v1)));
}

float Graphics::SampleHeightmapAt(float worldX, float worldZ) const
{
	if (!heightmapLoaded) return 0.0f;

	float terrainWorldSize = 0.36f;  
	float terrainScale = 40.0f;      
	float actualSize = terrainWorldSize * terrainScale;  

	float terrainCenterX = 8.0f;
	float terrainCenterZ = 8.0f;

	float minX = terrainCenterX - actualSize / 2;  
	float maxX = terrainCenterX + actualSize / 2;  
	float minZ = terrainCenterZ - actualSize / 2;  
	float maxZ = terrainCenterZ + actualSize / 2;  

	float u = (worldX - minX) / actualSize;
	float v = (worldZ - minZ) / actualSize;

	v = 1.0f - v;

	if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
		return 0.0f; 
	}

	float x = u * (heightmapWidth - 1);
	float y = v * (heightmapHeight - 1);

	int x0 = (int)floor(x);
	int y0 = (int)floor(y);
	int x1 = min(x0 + 1, heightmapWidth - 1);
	int y1 = min(y0 + 1, heightmapHeight - 1);

	float fx = x - x0;
	float fy = y - y0;

	float h00 = heightmapData[y0 * heightmapWidth + x0];
	float h10 = heightmapData[y0 * heightmapWidth + x1];
	float h01 = heightmapData[y1 * heightmapWidth + x0];
	float h11 = heightmapData[y1 * heightmapWidth + x1];

	float h0 = h00 * (1.0f - fx) + h10 * fx;
	float h1 = h01 * (1.0f - fx) + h11 * fx;
	float height = h0 * (1.0f - fy) + h1 * fy;

	return height * heightmapScale;
}

void Graphics::LoadHeightmapData(const std::wstring& filePath, int width, int height, float scale)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		OutputDebugStringA("Failed to load heightmap data for CPU sampling\n");
		return;
	}

	std::vector<UINT8> rawData(width * height);
	file.read(reinterpret_cast<char*>(rawData.data()), width * height);
	file.close();

	heightmapData.resize(width * height);
	for (size_t i = 0; i < rawData.size(); ++i) {
		heightmapData[i] = rawData[i] / 255.0f;
	}

	heightmapWidth = width;
	heightmapHeight = height;
	heightmapScale = scale;
	heightmapLoaded = true;

	OutputDebugStringA("Heightmap data loaded for CPU sampling\n");
}
