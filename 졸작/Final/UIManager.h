#pragma once
#include <DescriptorHeap.h>
#include <SpriteFont.h>
#include <ResourceUploadBatch.h>
#include <GraphicsMemory.h>
#include "UIComponent.h"

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

	void AddUIComponent(shared_ptr<UIComponent> comp);
	
	template<typename T>
	T* GetUIComponent(const wstring& name);

	void SetCurrentScene(SceneType scene) { currentScene = scene; }
	UITextureData* GetUITexture(const wstring& name);
	DescriptorHeap* GetUISrvHeap() const { return uiSrvHeap.get(); }

private:
	void RegisterFont(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterComponents();

private:
	unique_ptr<GraphicsMemory> graphicsMemory;
	unique_ptr<DescriptorHeap> uiSrvHeap;
	unique_ptr<SpriteBatch> spriteBatch;

	unordered_map<wstring, UIFontData> uiFontMap;
	unordered_map<wstring, UITextureData> uiTextureMap;

	unordered_map<SceneType, vector<shared_ptr<UIComponent>>> sceneUIMap;
	SceneType currentScene;

	static UINT nextIndex;
};

template<typename T>
inline T* UIManager::GetUIComponent(const wstring& name)
{
	for (auto& comp : sceneUIMap[currentScene])
	{
		if (T* casted = dynamic_cast<T*>(comp.get()))
		{
			if (casted->GetUIName() == name)
				return casted;
		}
	}

	return nullptr;
}
