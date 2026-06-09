#pragma once
#include <DescriptorHeap.h>
#include <SpriteFont.h>
#include <ResourceUploadBatch.h>
#include <GraphicsMemory.h>
#include "UIController.h"
#include "SceneManager.h"
#include "ScreenFade.h"

class DX12Core;
class VideoPlayer;

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
	UIManager();
	~UIManager();

	void Initialize(DX12Core& core);
	void Update(float deltaTime);
	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp);
	void Release();

	// 풀스크린 시네마틱 영상(mp4) 재생. 끝나면 자동 종료.
	void PlayVideo(const wstring& path);
	bool IsVideoPlaying() const { return videoPlaying; }

	void SetCurrentScene(SceneType scene) { currentScene = scene; }
	UITextureData* GetUITexture(const wstring& name);
	UIFontData* GetFont(const wstring& name);
	DescriptorHeap* GetUISrvHeap() const { return uiSrvHeap.get(); }

	template<typename T>
	T* GetController(SceneType type);

	template<typename T>
	T* GetController() { return GetController<T>(currentScene); }

	ScreenFade* GetScreenFade() const { return screenFade.get(); }

private:
	void RegisterFont(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload);
	void RegisterControllers();

private:
	unique_ptr<GraphicsMemory> graphicsMemory;
	unique_ptr<DescriptorHeap> uiSrvHeap;
	unique_ptr<SpriteBatch> spriteBatch;
	unique_ptr<ScreenFade> screenFade;

	unordered_map<wstring, UIFontData> uiFontMap;
	unordered_map<wstring, UITextureData> uiTextureMap;

	unordered_map<SceneType, unique_ptr<UIController>> controllers;

	DX12Core* coreRef = nullptr;
	unique_ptr<VideoPlayer> video;
	UINT videoHeapIndex = 0;
	bool videoPlaying = false;

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
