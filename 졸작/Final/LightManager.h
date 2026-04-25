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

struct ForwardLightConstants {
	XMFLOAT3 direction;
	float padding;
	XMFLOAT3 color;
	float intensity;
};

class LightManager
{
public:
	static constexpr UINT MAX_LIGHTS = 256;

	void Initialize(ID3D12Device* device);
	void UpdateLights();
	void SetSkyBox(SkyBox* sky) { skyBox = sky; }
	SkyBox* GetSkyBox() const { return skyBox; }

	UploadBuffer* GetDeferredLightCB() const;
	UploadBuffer* GetForwardLightCB() const;
	UploadBuffer* GetDeferredLightSB() const;

	// For Imgui
	DeferredLightConstants& GetDeferredLightData() { return deferredLightData; }
	ForwardLightConstants& GetForwardLightData() { return forwardLightData; }
	LightData* GetLights() { return lights.data(); }

private:
	void SetupLights();

private:
	DeferredLightConstants deferredLightData = {};
	ForwardLightConstants forwardLightData = {};
	std::vector<LightData> lights;

	unique_ptr<UploadBuffer> deferredLightCB;
	unique_ptr<UploadBuffer> forwardLightCB;
	unique_ptr<UploadBuffer> deferredLightSB;

	SkyBox* skyBox = nullptr;
};
