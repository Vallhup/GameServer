#include "pch.h"
#include "LightManager.h"
#include "SkyBox.h"

void LightManager::Initialize(ID3D12Device* device)
{
	deferredLightCB = make_unique<UploadBuffer>();
	forwardLightCB = make_unique<UploadBuffer>();
	deferredLightSB = make_unique<UploadBuffer>();

	deferredLightCB->Initialize(device, sizeof(DeferredLightConstants));
	forwardLightCB->Initialize(device, sizeof(ForwardLightConstants));
	deferredLightSB->Initialize(device, sizeof(LightData) * MAX_LIGHTS);

	lights.resize(MAX_LIGHTS);

	SetupLights();
}

void LightManager::UpdateLights()
{
	if (skyBox)
	{
		const SkySun& sun = skyBox->GetSun();
		lights[0].position = sun.direction;
		lights[0].color = sun.color;
		lights[0].intensity = sun.intensity;
		lights[0].type = 0;

		forwardLightData.direction = sun.direction;
		forwardLightData.color = sun.color;
		forwardLightData.intensity = sun.intensity;
	}

	deferredLightCB->CopyData(&deferredLightData, sizeof(DeferredLightConstants));
	forwardLightCB->CopyData(&forwardLightData, sizeof(ForwardLightConstants));
	deferredLightSB->CopyData(lights.data(), sizeof(LightData) * MAX_LIGHTS);
}

UploadBuffer* LightManager::GetDeferredLightCB() const
{
	return deferredLightCB.get();
}

UploadBuffer* LightManager::GetForwardLightCB() const
{
	return forwardLightCB.get();
}

UploadBuffer* LightManager::GetDeferredLightSB() const
{
	return deferredLightSB.get();
}

void LightManager::SetupLights()
{
	forwardLightData = { {0, 0, -1}, 0, {1, 1, 1}, 0.25f };

	deferredLightData.lightCount = 23;

	lights[0] = {
		{-0.75f, -1.07f, -1.0f}, 0,
		{1, 1, 1}, 1.0f,
		0,
		{0, 0, 0}
	};
	lights[1] = {
		{0, 0, 1}, 0,
		{1, 1, 1}, 0.0f,
		0,
		{0, 0, 0}
	};

	lights[2] = {
		{481.f, 25.f, 482.0f}, 50.0f,
		{1, 1, 1}, 0.0f,
		1,
		{0, 0, 0}
	};

	// Point Lights (3~22)
	float spacing = 15.0f;
	float height = 4.0f;
	float leftX = -7.0f;
	float rightX = 7.0f;

	for (int i = 3; i < 23; ++i) {
		int lightIndex = i - 3;
		int rowIndex = lightIndex % 10;
		bool isLeftRow = (lightIndex < 10);

		float x = isLeftRow ? leftX : rightX;
		float z = -(rowIndex * spacing);

		XMFLOAT3 color = { 1.0f, 0.25f, 0.0f };

		lights[i] = {
			{x, height, z + 70.0f}, 10.0f,
			color, 0.0f,
			1,
			{0, 0, 0}
		};
	}

	UpdateLights();
}
