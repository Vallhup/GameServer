#pragma once
#include "Scene.h"

class SkyBox;
class BeaconLightComponent;

enum class BeaconCine { None, FadeOut, Rising, Growing, Showcase, Done };

struct BeaconCinematicConfig
{
	XMFLOAT3 riseStart;
	XMFLOAT3 riseEnd;
	float    riseVerticalRatio;  

	float fadeDur;
	float riseDur;
	float growDur;
	float brightenDur;
	float showcaseHoldDur;

	float beaconBaseSize;
	float beaconMaxSize;

	XMFLOAT2 camEyeXZ;    
	float    camYAbove;
	float    camBack;     
	XMFLOAT3 lookTarget;  

	float sunMult;
	float skySatMult;
	float skyExpMult;

	XMFLOAT2 scatterCenter;
	XMFLOAT2 scatterSpan;
	int      scatterNX;
	int      scatterNZ;
	float    scatterLayerY;

	const wchar_t* burstEffect;
	const wchar_t* atmosphereEffect;
};

class BeaconCinematicScene : public Scene
{
protected:
	virtual const BeaconCinematicConfig& GetCinematicConfig() const = 0;
	virtual float SampleHeightAt(float worldX, float worldZ) const = 0;

	bool IsCinematicActive() const { return cineState != BeaconCine::None; }
	void StartBeaconCinematic();
	void UpdateBeaconCinematic(float deltaTime);

	void OnBossDefeated() override;
	void OnBeaconCinematicStart() override;

private:
	void UpdateCinematicCamera(const XMFLOAT3& look);
	void ScatterAtmosphere();
	void CaptureBrightenBase();
	void ApplyBrighten(float t);

protected:
	SkyBox* cineSkyBox = nullptr;
	BeaconLightComponent* beaconLight = nullptr;
	XMFLOAT3 beaconSpawnPos = {};                 

	BeaconCine cineState = BeaconCine::None;
	float      cineTimer = 0.0f;
	XMFLOAT3   beaconCinePos = {};
	float      beaconCineSize = 0.0f;
	float      cineSunBase = 1.0f;
	float      cineSkySatBase = 1.0f;
	float      cineSkyExpBase = 1.0f;
	vector<int> atmosphereHandles;
};
