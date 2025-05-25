#include "pch.h"
#include "Graphics.h"
#include "GameObject.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "VertexIndexBuffer.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "DX12Graphics.h"

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

void Graphics::DrawMesh(const XMFLOAT4X4& view, const XMFLOAT4X4& projection,
	const Viewport& viewport, const XMMATRIX& world, const GameObject* obj, UINT objIndex) const
{
	XMMATRIX matView = XMLoadFloat4x4(&view);
	XMMATRIX matProj = XMLoadFloat4x4(&projection);
	matView = XMMatrixTranspose(matView);
	matProj = XMMatrixTranspose(matProj);
	GET(DX12Graphics).GetFrameCB()->CopyData(&matView, sizeof(XMMATRIX), 0); 
	GET(DX12Graphics).GetFrameCB()->CopyData(&matProj, sizeof(XMMATRIX), sizeof(XMMATRIX)); 

	ID3D12GraphicsCommandList* cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
	cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());
	if (!obj->GetTransParent())
		cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
	else
		cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetTransparentPSO());
	cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
	obj->GetMesh()->Bind(cmdList);

	XMMATRIX matWorld = XMMatrixTranspose(world);
	const UINT constantBufferOffset = objIndex * 256;
	GET(DX12Graphics).GetSceneCB()->CopyData(&matWorld, sizeof(XMMATRIX), constantBufferOffset); 
	cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + constantBufferOffset);
	obj->GetMesh()->Draw(cmdList);

	char buffer[256];
	sprintf_s(buffer, "Total objects: %d (limit: 512)\n", objIndex);
	OutputDebugStringA(buffer);
}

void Graphics::DrawObjectsToWorld(const GameObject* obj, const XMFLOAT4X4& view, const XMFLOAT4X4& projection,
	const Viewport& viewport, const XMMATRIX& parentWorld, UINT& currentIndex) const
{
	if (!obj) return;

	XMMATRIX world = obj->GetTransform().CreateWorldMatrix() * parentWorld;

	if (obj->GetActive() && obj->GetMesh() != nullptr)
	{
		DrawMesh(view, projection, viewport, world, obj, currentIndex);
		currentIndex++;
	}
	if (obj->GetSibling())
	{
		DrawObjectsToWorld(obj->GetSibling(), view, projection, viewport, parentWorld, currentIndex);
	}
	if (obj->GetChild())
	{
		DrawObjectsToWorld(obj->GetChild(), view, projection, viewport, world, currentIndex);
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