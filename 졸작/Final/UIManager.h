#pragma once
#include <DescriptorHeap.h>
#include <SpriteFont.h>
#include <ResourceUploadBatch.h>
#include <GraphicsMemory.h>
#include "UIController.h"
#include "SceneManager.h"

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
	void Update(float deltaTime);
	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp);
	void Release();

	void SetCurrentScene(SceneType scene) { currentScene = scene; }
	UITextureData* GetUITexture(const wstring& name);
	UIFontData* GetFont(const wstring& name);
	DescriptorHeap* GetUISrvHeap() const { return uiSrvHeap.get(); }

	template<typename T>
	T* GetController(SceneType type);

	template<typename T>
	T* GetController() { return GetController<T>(currentScene); }

private:
	void RegisterFont(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterControllers();

private:
	unique_ptr<GraphicsMemory> graphicsMemory;
	unique_ptr<DescriptorHeap> uiSrvHeap;
	unique_ptr<SpriteBatch> spriteBatch;

	unordered_map<wstring, UIFontData> uiFontMap;
	unordered_map<wstring, UITextureData> uiTextureMap;

	unordered_map<SceneType, unique_ptr<UIController>> controllers;

	SceneType currentScene;

	static constexpr int MAX_RESOURCE_COUNT = 96;

	static UINT nextIndex;
};

template<typename T>
inline T* UIManager::GetController(SceneType type)
{
	auto it = controllers.find(type);
	if (it != controllers.end())
		return static_cast<T*>(it->second.get());
	return nullptr;
}
