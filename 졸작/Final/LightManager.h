#pragma once

class SkyBox;

struct LightData {
	XMFLOAT3 position;
	float range;
	XMFLOAT3 color;
	float intensity;
	int type;             // 0=directional, 1=point
	XMFLOAT3 padding;
};

struct DeferredLightConstants {
	int lightCount;
	XMFLOAT3 padding;
};

class LightManager
{
public:
	static constexpr UINT MAX_LIGHTS = 512;

	void Initialize(ID3D12Device* device);
	void UpdateLights();

	void LoadSceneLights(const wstring& pointLightFile, bool enableDirectional);

	bool LoadFromFile(const wstring& path, int startSlot = 1);
	static vector<XMFLOAT3> LoadCandlePositions(const wstring& path);
	void SetSkyBox(SkyBox* sky) { skyBox = sky; }
	SkyBox* GetSkyBox() const { return skyBox; }

	UploadBuffer* GetDeferredLightCB() const;
	UploadBuffer* GetDeferredLightSB() const;

	// For Imgui
	DeferredLightConstants& GetDeferredLightData() { return deferredLightData; }
	LightData* GetLights() { return lights.data(); }

private:
	DeferredLightConstants deferredLightData = {};
	vector<LightData> lights;

	bool useDirectional = true;

	unique_ptr<UploadBuffer> deferredLightCB;
	unique_ptr<UploadBuffer> deferredLightSB;

	SkyBox* skyBox = nullptr;
};
