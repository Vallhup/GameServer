#pragma once
#include "Scene.h"

class Terrain;
class SkyBox;
class Water;
class BeaconLightComponent;

enum class BeaconCine { None, FadeOut, Rising, Growing, Showcase, Done };

class FirstBattleScene final : public Scene
{
public:
	FirstBattleScene() = default;
	FirstBattleScene(const FirstBattleScene&) = delete;
	FirstBattleScene& operator=(const FirstBattleScene&) = delete;
	~FirstBattleScene() = default;

	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadowStatic() override;
	void RenderSceneShadowDynamic() override;
	void RenderSceneEffects() override;

	void Release() override;

	SceneSettings GetSceneSettings() const override;

protected:
	void InitializeLogic() override;
	void InitializeSceneEnvironments() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;
	void RequestSceneChange() override;

	const char* GetBGMPath() const override;

private:
	float SampleHeightAt(float worldX, float worldZ) const;

	void UpdateBeaconCinematic(float deltaTime);
	void UpdateCinematicCamera(const XMFLOAT3& look);
	void ScatterAtmosphere();
	void CaptureBrightenBase();
	void ApplyBrighten(float t);

private:
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Terrain> terrain;
	shared_ptr<Terrain> oceanFloor;
	shared_ptr<Water> water;

	BeaconLightComponent* beaconLight = nullptr;
	BeaconCine cineState = BeaconCine::None;
	float      cineTimer = 0.0f;
	XMFLOAT3   beaconCinePos = { 335.237946f, 77.0f, 590.663147f };
	float      beaconCineSize = 2.0f;
	float      cineSunBase = 1.0f;
	float      cineSkySatBase = 1.0f;
	float      cineSkyExpBase = 1.0f;
	vector<int> atmosphereHandles;   

	static constexpr float CINE_FADE_DUR        = 1.0f;
	static constexpr float CINE_RISE_DUR        = 6.0f;    
	static constexpr float CINE_RISE_HEIGHT     = 20.0f;   
	static constexpr float CINE_GROW_DUR        = 1.5f;
	static constexpr float CINE_BEACON_BASE_SIZE = 2.0f;
	static constexpr float CINE_BEACON_MAX_SIZE  = 8.0f;
	static constexpr float CINE_BRIGHTEN_DUR      = 2.5f;  
	static constexpr float CINE_SHOWCASE_HOLD_DUR = 8.0f;  
	static constexpr float CINE_CAM_EYE_X   = 352.913971f;
	static constexpr float CINE_CAM_EYE_Z   = 586.207336f;
	static constexpr float CINE_CAM_Y_ABOVE = 20.0f;
	static constexpr float CINE_CAM_BACK    = 30.0f;   
	static constexpr float CINE_LOOK_X = 243.137360f;
	static constexpr float CINE_LOOK_Y = 58.121223f;
	static constexpr float CINE_LOOK_Z = 606.244629f;
	static constexpr float CINE_SUN_MULT        = 16.0f;
	static constexpr float CINE_SKY_SAT_MULT    = 2.0f;
	static constexpr float CINE_SKY_EXP_MULT    = 1.5f;
};
