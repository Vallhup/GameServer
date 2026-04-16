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
	LightData lights[23]; 
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
	void Initialize(ID3D12Device* device);
	void UpdateLights();
	void SetSkyBox(SkyBox* sky) { skyBox = sky; }

	UploadBuffer* GetDeferredLightCB() const;
	UploadBuffer* GetForwardLightCB() const;

	// For Imgui
	DeferredLightConstants& GetDeferredLightData() { return deferredLightData; }
	ForwardLightConstants& GetForwardLightData() { return forwardLightData; }

private:
	void SetupLights();

private:
	DeferredLightConstants deferredLightData = {};
	ForwardLightConstants forwardLightData = {};

	unique_ptr<UploadBuffer> deferredLightCB;
	unique_ptr<UploadBuffer> forwardLightCB;

	SkyBox* skyBox = nullptr;
};

