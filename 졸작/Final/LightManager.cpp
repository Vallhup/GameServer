#include "pch.h"
#include "LightManager.h"
#include "SkyBox.h"

void LightManager::Initialize(ID3D12Device* device)
{
	deferredLightCB = make_unique<UploadBuffer>();
	deferredLightSB = make_unique<UploadBuffer>();

	deferredLightCB->Initialize(device, sizeof(DeferredLightConstants));
	deferredLightSB->Initialize(device, sizeof(LightData) * MAX_LIGHTS);

	lights.resize(MAX_LIGHTS);
}

void LightManager::UpdateLights()
{
	if (useDirectional && skyBox)
	{
		const SkySun& sun = skyBox->GetSun();
		lights[0].position = sun.direction;
		lights[0].color = sun.color;
		lights[0].intensity = sun.intensity;
		lights[0].type = 0;
	}

	deferredLightCB->CopyData(&deferredLightData, sizeof(DeferredLightConstants));
	deferredLightSB->CopyData(lights.data(), sizeof(LightData) * MAX_LIGHTS);
}

UploadBuffer* LightManager::GetDeferredLightCB() const
{
	return deferredLightCB.get();
}

UploadBuffer* LightManager::GetDeferredLightSB() const
{
	return deferredLightSB.get();
}

bool LightManager::LoadFromFile(const wstring& path, int startSlot)
{
	ifstream file(path.c_str());
	if (!file.is_open())
		return false;

	int slot = startSlot;
	string line;
	while (getline(file, line) && slot < static_cast<int>(MAX_LIGHTS))
	{
		if (line.empty())
			continue;

		istringstream iss(line);
		float px, py, pz, r, g, b, intensity, range;
		if (!(iss >> px >> py >> pz >> r >> g >> b >> intensity >> range))
			continue;

		lights[slot] = {
			{ px, py, pz }, range,
			{ r, g, b }, intensity * 0.25f,
			1,
			{ 0, 0, 0 }
		};
		++slot;
	}

	deferredLightData.lightCount = slot;
	return true;
}

void LightManager::LoadSceneLights(const wstring& pointLightFile, bool enableDirectional)
{
	useDirectional = enableDirectional;

	lights.assign(MAX_LIGHTS, {});

	lights[0] = {
		{ -0.43f, -0.62f, -0.58f }, 0,
		{ 1, 1, 1 }, enableDirectional ? 1.0f : 0.0f,
		0,
		{ 0, 0, 0 }
	};
	deferredLightData.lightCount = 1;

	LoadFromFile(pointLightFile, 1);
}
