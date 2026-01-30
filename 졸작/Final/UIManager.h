#pragma once
#include <DescriptorHeap.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <ResourceUploadBatch.h>
#include <GraphicsMemory.h>

class DX12Core;

class UIManager
{
public:
	void Initialize(DX12Core& core);
	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp);
	void Release();

private:
	unique_ptr<GraphicsMemory> graphicsMemory;
	unique_ptr<DescriptorHeap> uiSrvHeap;
	unique_ptr<SpriteBatch> spriteBatch;
	unique_ptr<SpriteFont> spriteFont;
	ComPtr<ID3D12Resource> statusTexture;
};
