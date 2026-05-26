#include "pch.h"
#include "BeaconCinematicScene.h"
#include "Engine.h"
#include "UIManager.h"
#include "EffectManager.h"
#include "SkyBox.h"
#include "LightManager.h"
#include "BeaconLightComponent.h"
#include "Input.h"

void BeaconCinematicScene::StartBeaconCinematic()
{
	const auto& cfg = GetCinematicConfig();

	cineState = BeaconCine::FadeOut;
	cineTimer = 0.0f;
	INPUT.SetBlocked(true);

	for (auto& batch : instancingBatches)
		batch->SetCinematicMode(true);

	for (const auto& [mid, mtype] : activeMonsterTypes)
		if (auto it = activeCharacters.find(mid); it != activeCharacters.end())
			it->second->SetId(-1);

	if (auto* fade = ENGINE.GetUIManager()->GetScreenFade())
		fade->FadeOut(cfg.fadeDur);
}

void BeaconCinematicScene::UpdateCinematicCamera(const XMFLOAT3& look)
{
	if (!cam) return;
	const auto& cfg = GetCinematicConfig();

	float bx = cfg.camEyeXZ.x - cfg.lookTarget.x;
	float bz = cfg.camEyeXZ.y - cfg.lookTarget.z;
	const float bl = sqrtf(bx * bx + bz * bz);
	if (bl > 0.0001f) { bx /= bl; bz /= bl; }

	const XMFLOAT3 eye{
		cfg.camEyeXZ.x + bx * cfg.camBack,
		beaconCinePos.y + cfg.camYAbove,
		cfg.camEyeXZ.y + bz * cfg.camBack };
	cam->SetCinematicView(*coreRef, eye, look);
}

void BeaconCinematicScene::ScatterAtmosphere()
{
	const auto& cfg = GetCinematicConfig();

	const float startX = cfg.scatterCenter.x - cfg.scatterSpan.x * 0.5f;
	const float startZ = cfg.scatterCenter.y - cfg.scatterSpan.y * 0.5f;
	const float stepX  = cfg.scatterSpan.x / (cfg.scatterNX - 1);
	const float stepZ  = cfg.scatterSpan.y / (cfg.scatterNZ - 1);

	for (int i = 0; i < cfg.scatterNX; ++i)
		for (int j = 0; j < cfg.scatterNZ; ++j)
		{
			const float x = startX + i * stepX;
			const float z = startZ + j * stepZ;
			const float ground = SampleHeightAt(x, z);
			atmosphereHandles.push_back(
				EFFECT_MANAGER->Play(cfg.atmosphereEffect, { x, ground + cfg.scatterLayerY, z }));
		}
}

void BeaconCinematicScene::CaptureBrightenBase()
{
	if (!cineSkyBox) return;
	cineSunBase = cineSkyBox->GetSun().intensity;
	auto& sc = cineSkyBox->GetConstants();
	cineSkySatBase = sc.skySaturation;
	cineSkyExpBase = sc.skyExposure;
}

void BeaconCinematicScene::ApplyBrighten(float t)
{
	if (!cineSkyBox) return;
	const auto& cfg = GetCinematicConfig();

	cineSkyBox->GetSun().intensity = cineSunBase + (cineSunBase * cfg.sunMult - cineSunBase) * t;
	coreRef->GetLightMgr()->UpdateLights();

	auto& sc = cineSkyBox->GetConstants();
	sc.skySaturation = cineSkySatBase + (cineSkySatBase * cfg.skySatMult - cineSkySatBase) * t;
	sc.skyExposure   = cineSkyExpBase + (cineSkyExpBase * cfg.skyExpMult - cineSkyExpBase) * t;
	cineSkyBox->UpdateConstants();
}

void BeaconCinematicScene::UpdateBeaconCinematic(float deltaTime)
{
	const auto& cfg = GetCinematicConfig();
	auto* fade = ENGINE.GetUIManager()->GetScreenFade();
	cineTimer += deltaTime;

	switch (cineState)
	{
	case BeaconCine::FadeOut:
		if (fade && fade->IsBlack())
		{
			cineState = BeaconCine::Rising;
			cineTimer = 0.0f;
			beaconCinePos = cfg.riseStart;
			beaconCineSize = cfg.beaconBaseSize;
			if (beaconLight) { beaconLight->SetPosition(beaconCinePos); beaconLight->SetSize(beaconCineSize); }
			UpdateCinematicCamera(beaconCinePos);
			fade->FadeIn(cfg.fadeDur);
		}
		break;

	case BeaconCine::Rising:
	{
		const float t = min(cineTimer / cfg.riseDur, 1.0f);
		const float vr = cfg.riseVerticalRatio;
		if (t <= vr)
		{
			const float tv = (vr > 0.0f) ? t / vr : 1.0f;
			beaconCinePos = {
				cfg.riseStart.x,
				cfg.riseStart.y + (cfg.riseEnd.y - cfg.riseStart.y) * tv,
				cfg.riseStart.z };
		}
		else
		{
			const float th = (vr < 1.0f) ? (t - vr) / (1.0f - vr) : 1.0f;
			beaconCinePos = {
				cfg.riseStart.x + (cfg.riseEnd.x - cfg.riseStart.x) * th,
				cfg.riseEnd.y,
				cfg.riseStart.z + (cfg.riseEnd.z - cfg.riseStart.z) * th };
		}
		if (beaconLight) beaconLight->SetPosition(beaconCinePos);
		UpdateCinematicCamera(beaconCinePos);
		if (t >= 1.0f) { cineState = BeaconCine::Growing; cineTimer = 0.0f; }
		break;
	}

	case BeaconCine::Growing:
	{
		const float t = min(cineTimer / cfg.growDur, 1.0f);
		beaconCineSize = cfg.beaconBaseSize + (cfg.beaconMaxSize - cfg.beaconBaseSize) * t;
		if (beaconLight) beaconLight->SetSize(beaconCineSize);
		UpdateCinematicCamera(beaconCinePos);
		if (t >= 1.0f)
		{
			EFFECT_MANAGER->Play(cfg.burstEffect, beaconCinePos);
			if (beaconLight) beaconLight->Stop();
			ScatterAtmosphere();
			CaptureBrightenBase();
			cineState = BeaconCine::Showcase;
			cineTimer = 0.0f;
		}
		break;
	}

	case BeaconCine::Showcase:
	{
		const float t = min(cineTimer / cfg.brightenDur, 1.0f);
		const XMFLOAT3 look{
			beaconCinePos.x + (cfg.lookTarget.x - beaconCinePos.x) * t,
			beaconCinePos.y + (cfg.lookTarget.y - beaconCinePos.y) * t,
			beaconCinePos.z + (cfg.lookTarget.z - beaconCinePos.z) * t };
		UpdateCinematicCamera(look);
		ApplyBrighten(t);
		if (cineTimer >= cfg.brightenDur + cfg.showcaseHoldDur)
		{
			if (fade)
			{
				const auto handles = atmosphereHandles;
				atmosphereHandles.clear();
				fade->SetOnFadedOut([handles]() {
					for (int h : handles)
						EFFECT_MANAGER->Stop(h);

					auto& tr = ENGINE.GetWorldTransitionController();
					const uint32_t rid = tr.CreateRequestId();
					if (tr.BeginRequest(rid))
						if (!NETWORK_MANAGER->SendWorldTransitionRequestPacket(rid))
							tr.Reset();
				});
				fade->FadeOut(cfg.fadeDur);
			}
			cineState = BeaconCine::Done;
		}
		break;
	}

	case BeaconCine::Done:
	default:
		break;
	}
}
