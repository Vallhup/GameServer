#include "pch.h"
#include "Water.h"
#include "DX12Core.h"
#include "VertexIndexBuffer.h"
#include "SwapChain.h"
#include "RenderTargets.h"

void Water::Initialize(DX12Core& core)
{
	BuildVertices();
	BuildIndices();
	CreateReflectionResources(core.GetDevice());
	CreateRefractionResources(core.GetDevice());
	CreateRenderTargetView(core.GetDevice());

	vertexIndexBuffer = make_shared<VertexIndexBuffer>();
	vertexIndexBuffer->Initialize(
		core.GetDevice(),
		core.GetGraphicsCmdList(),
		vertices,
		indices
	);

	objectCB = make_unique<UploadBuffer>();
	objectCB->Initialize(core.GetDevice(), CONSTANT_BUFFER_ALIGNMENT);

	ObjectConstants obj = {};
	obj.world = XMMatrixTranspose(XMMatrixIdentity());
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::Render(DX12Core& core, ID3D12GraphicsCommandList* cmdList)
{
	CopyBackBuffer(core);

	if (vertexIndexBuffer)
	{
		vertexIndexBuffer->Bind(cmdList);
		vertexIndexBuffer->Draw(cmdList);
	}
}

void Water::SetPosition(float x, float y, float z)
{
	position = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX transMat = XMMatrixTranslation(x, y, z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::SetScale(float x, float y, float z)
{
	scale = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(x, y, z);
	XMMATRIX transMat = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::BindReflectionRT(ID3D12GraphicsCommandList* cmdList)
{
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cmdList->ClearRenderTargetView(reflectionRTVHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(reflectionDepthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &reflectionRTVHandle, FALSE, &reflectionDepthHandle);
}

void Water::BindRefractionRT(ID3D12GraphicsCommandList* cmdList)
{
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cmdList->ClearRenderTargetView(refractionRTVHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(refractionDepthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &refractionRTVHandle, FALSE, &refractionDepthHandle);
}

void Water::BuildVertices()
{
	vertices.clear();
	vertices.reserve(4);

	Vertex v0 = {}, v1 = {}, v2 = {}, v3 = {};
	
	v0.pos = XMFLOAT3(1.0f, 0.0f, 1.0f);
	v1.pos = XMFLOAT3(1.0f, 0.0f, -1.0f);
	v2.pos = XMFLOAT3(-1.0f, 0.0f, -1.0f);
	v3.pos = XMFLOAT3(-1.0f, 0.0f, 1.0f);

	v0.uv = XMFLOAT2(1.0f, 0.0f);  // 우상
	v1.uv = XMFLOAT2(1.0f, 1.0f);  // 우하
	v2.uv = XMFLOAT2(0.0f, 1.0f);  // 좌하
	v3.uv = XMFLOAT2(0.0f, 0.0f);  // 좌상

	v0.normal = v1.normal = v2.normal = v3.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	v0.tangent = v1.tangent = v2.tangent = v3.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
	v0.color = v1.color = v2.color = v3.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
}

void Water::BuildIndices()
{
	// 0 1 2 & 0 2 3
	indices.clear();
	indices.reserve(6);

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	indices.push_back(0);
	indices.push_back(2);
	indices.push_back(3);
}

void Water::CreateReflectionResources(ID3D12Device* device)
{
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, WinSize.x, WinSize.y);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&reflectionDSVBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&reflectionDepthHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	reflectionDepthHandle = reflectionDepthHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(reflectionDSVBuffer.Get(), &dsvDesc, reflectionDepthHandle);

	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};

	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = rtDesc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&reflectionRT));
	MASSERT(SUCCEEDED(hr), "Failed to create reflection RT");

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&reflectionSRVHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = reflectionSRVHeap->GetCPUDescriptorHandleForHeapStart();
	reflectionSRVHandle = reflectionSRVHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(reflectionRT.Get(), &srvDesc, srvCpuHandle);

	OutputDebugStringA("Reflection resources created!!\n");
}

void Water::CreateRefractionResources(ID3D12Device* device)
{
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, WinSize.x, WinSize.y);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&refractionDepthTexture));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&refractionDepthHeap));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&refractionSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create refraction SRV Heap");

	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = refractionSRVHeap->GetCPUDescriptorHandleForHeapStart();
	refractionSRVHandle = refractionSRVHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(refractionDepthTexture.Get(), &srvDesc, srvCpuHandle);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	refractionDepthHandle = refractionDepthHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(refractionDepthTexture.Get(), &dsvDesc, refractionDepthHandle);

	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};

	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = rtDesc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&refractionRT));
	MASSERT(SUCCEEDED(hr), "Failed to create refraction RT");

	OutputDebugStringA("Refraction resources created!!\n");
}

void Water::CreateRenderTargetView(ID3D12Device* device)
{
	int rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&reflectionRTVHeap));
	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&refractionRTVHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = reflectionRTVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin2 = refractionRTVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvViewDesc = {};
	rtvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	reflectionRTVHandle = rtvHeapBegin;
	refractionRTVHandle = rtvHeapBegin2;

	device->CreateRenderTargetView(reflectionRT.Get(), &rtvViewDesc, reflectionRTVHandle);
	device->CreateRenderTargetView(refractionRT.Get(), &rtvViewDesc, refractionRTVHandle);

	OutputDebugStringA("RTVs created!!\n");
}

void Water::CopyBackBuffer(DX12Core& core)
{
	auto cmdList = core.GetGraphicsCmdList();
	ID3D12Resource* backBuffer = core.GetSwapChainMgr()->GetCurrentBuffer();

	D3D12_RESOURCE_BARRIER barriers[2];
	static bool firstFrame = true;

	// 백버퍼: RENDER_TARGET → COPY_SOURCE
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	// reflectionRT: RENDER_TARGET → COPY_DEST
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		reflectionRT.Get(),
		firstFrame ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	if (firstFrame) firstFrame = false;

	cmdList->ResourceBarrier(2, barriers);

	// 복사
	cmdList->CopyResource(reflectionRT.Get(), backBuffer);

	// 상태 복원
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		reflectionRT.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	cmdList->ResourceBarrier(2, barriers);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = core.GetSwapChainMgr()->GetCurrentRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = core.GetRenderTargetMgr()->GetDSVHandle();
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}