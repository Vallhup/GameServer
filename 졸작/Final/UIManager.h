#pragma once
#include <DescriptorHeap.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <ResourceUploadBatch.h>
#include <GraphicsMemory.h>

class DX12Core;

struct UIFontData {
	UINT heapIndex;
	unique_ptr<SpriteFont> font;
};

struct UITextureData {
	UINT heapIndex;
	ComPtr<ID3D12Resource> resource;
};

class UIManager
{
public:
	void Initialize(DX12Core& core);
	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp);
	void Release();

	void RegisterFont(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);

private:
	unique_ptr<GraphicsMemory> graphicsMemory;
	unique_ptr<DescriptorHeap> uiSrvHeap;
	unique_ptr<SpriteBatch> spriteBatch;

	unordered_map<wstring, UIFontData> uiFontMap;
	unordered_map<wstring, UITextureData> uiTextureMap;

	static UINT nextIndex;
};
