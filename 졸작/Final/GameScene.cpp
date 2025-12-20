#include "pch.h"
#include "GameScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Animator.h"
#include "Material.h"
#include "Camera.h"
#include "EffectRenderer.h"
#include "Engine.h"
#include "NetworkManager.h"

GameScene::~GameScene() = default;

void GameScene::CreateKnightPool()
{
	for (int i = 0; i < MAX_KNIGHT_COUNT; ++i)
	{
		auto knight = make_shared<MainCharacter>();
		knight->SetId(-1);
		auto meshRenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
		transform->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f);
		transform->SetRotation(-1.57f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		knightPool.push_back(knight);
		AddGameObject(knight);
	}
}

shared_ptr<GameObject> GameScene::CreateStaticMesh(const wstring& path, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale)
{
	auto obj = make_shared<GameObject>();
	obj->SetId(0);
	auto meshRenderer = obj->AddComponent<MeshRenderer>();
	auto transform = obj->AddComponent<Transform>();
	meshRenderer->SetMesh(*coreRef, path);
	transform->SetInitPosition(pos.x, pos.y, pos.z);
	transform->SetRotation(rot.x, rot.y, rot.z);
	transform->SetScale(scale.x, scale.y, scale.z);
	return obj;
}

void GameScene::CreateCastle()
{
	struct castleData {
		XMFLOAT3 position;
		XMFLOAT3 rotation;
		XMFLOAT3 scale;
	};

#pragma region Initialize CASTLEWALL
	for (int i = 2; i < 20; ++i)
	{
		wstring meshName = (i < 10) ? L"../FBXOutput/mesh_0" + to_wstring(i) : L"../FBXOutput/mesh_" + to_wstring(i);
		AddGameObject(CreateStaticMesh(meshName));
	}
#pragma endregion

#pragma region Initialize PILLARS
	constexpr castleData pillarData[] = {
		{{5.62315f, -0.016064f, -9.59924f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, -9.59924f   },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, -9.59924f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, -9.59924f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, 0.831307f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, 0.831307f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, 0.831307f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, 0.831307f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, 11.2619f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, 11.2619f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, 11.2619f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, 11.2619f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, 21.8516f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, 21.8516f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, 21.8516f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, 21.8516f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, 32.2828f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, 32.2828f	  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, 32.2828f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, 32.2828f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{26.9762f, -0.016064f, -9.59924f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-26.9815f, -0.016064f, -9.59924f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, -35.3441f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, -35.3441f   },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, -35.3441f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, -35.3441f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{26.9762f, -0.016064f, -35.3441f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-26.9815f, -0.016064f, -35.3441f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.62315f, -0.016064f, -45.8333f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.504f, -0.016064f, -45.8333f   },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5107f, -0.016064f, -45.8333f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-5.62991f, -0.016064f, -45.8333f },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-34.657f, -0.016064f, -27.727f   },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-34.657f, -0.016064f, -17.2682f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{34.5867f, -0.016064f, -27.727f   },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{34.5867f, -0.016064f, -17.2682f  },  {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : pillarData)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_pillar", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize FLOORS
	constexpr castleData floorData[] = {
		{{-11.0406f, 0.0f, 27.0283f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, 27.0283f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, 27.0283f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, 16.5593f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, 16.5593f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, 16.5593f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, 6.09022f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, 6.09022f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, 6.09022f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, -4.37883f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, -4.37883f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, -4.37883f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, -14.8479f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, -14.8479f},	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, -14.8479f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, -25.3169f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, -25.3169f},	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, -25.3169f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, -35.7861f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, -35.7861f},	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, -35.7861f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-11.0406f, 0.0f, -46.2553f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-0.172962f, 0.0f, -46.2553f},	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{10.6946f, 0.0f, -46.2553f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-32.7753f, 0.0f, -14.8479f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-21.9077f, 0.0f, -14.8479f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-32.7753f, 0.0f, -25.3169f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-21.9077f, 0.0f, -25.3169f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-32.7753f, 0.0f, -35.7861f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-21.9077f, 0.0f, -35.7861f },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{21.561f, 0.0f, -14.8479f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{32.4286f, 0.0f, -14.8479f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{21.561f, 0.0f, -25.3169f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{32.4286f, 0.0f, -25.3169f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{21.561f, 0.0f, -35.7861f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{32.4286f, 0.0f, -35.7861f	 },	{0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : floorData)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_01", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize CANDLES
	constexpr castleData candleData[] = {
		{{3.56737f, 0.0f, 0.790788f}, {0.0f, 3.14159f, 0.0f}, {0.01f, 0.00945804f, 0.01f}},
		{{-3.62466f, 0.0f, 0.790788f}, {0.0f, -0.349066f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{3.56737f, 0.0f, 11.3746f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-3.62466f, 0.0f, 11.3746f}, {0.0f, -0.610865f, 0.0f}, {0.0106772f, 0.0106772f, 0.0106772f}},
		{{3.56737f, 0.0f, 21.7258f}, {0.0f, -0.261799f, 0.0f}, {0.01f, 0.0112763f, 0.01f}},
		{{-3.62466f, 0.0f, 21.7258f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{3.56737f, 0.0f, -9.55116f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.0116163f, 0.01f}},
		{{-3.62466f, 0.0f, -9.55116f}, {0.0f, 0.261799f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{3.56737f, 0.0f, -35.2663f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.0129939f, 0.01f}},
		{{-3.62466f, 0.0f, -35.2663f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.0127554f, 0.01f}},
		{{-5.69569f, 0.0f, -33.0653f}, {0.0f, 0.374506f, 0.0f}, {0.01f, 0.00865495f, 0.01f}},
		{{-5.69569f, 0.0f, -12.068f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.47713f, 0.0f, -33.0653f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{5.47713f, 0.0f, -12.068f}, {0.0f, -0.192159f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.5294f, 0.0f, -33.0653f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{16.5294f, 0.0f, -12.068f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{26.2041f, 0.0f, -33.0653f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{26.2041f, 0.0f, -12.068f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5124f, 0.0f, -33.0653f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-16.5124f, 0.0f, -12.068f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-26.3291f, 0.0f, -33.3225f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-26.2811f, 0.201421f, -11.0413f}, {-1.45417f, 1.51516f, -0.495437f}, {0.01f, 0.01f, 0.01f}},
		{{-32.3072f, 0.0f, -17.9463f}, {0.0f, 0.287839f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-32.6318f, 0.0f, -27.223f}, {0.0f, -0.320081f, 0.0f}, {0.01f, 0.0112198f, 0.01f}},
		{{31.8466f, 0.0f, -17.3366f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{32.3281f, 0.0f, -27.4548f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-7.79683f, 0.0f, -9.63731f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-14.0193f, 0.0f, -9.63731f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{14.1051f, 0.0f, -9.63731f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{7.88262f, 0.0f, -9.63731f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-7.79683f, 0.0f, 0.872659f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-14.0193f, 0.0f, 0.872659f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{14.1051f, 0.0f, 0.872659f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{7.88262f, 0.0f, 0.872659f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-7.79683f, 0.0f, 11.2518f}, {0.0f, 0.174533f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-14.9076f, 0.231538f, 11.2518f}, {0.0f, 0.0f, -1.45794f}, {0.01f, 0.01f, 0.01f}},
		{{14.1051f, 0.0f, 11.2518f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{7.88262f, 0.0f, 11.2518f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-7.79683f, 0.0f, 21.7484f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{-14.0193f, 0.0f, 21.7484f}, {0.0f, 0.314048f, 0.0f}, {0.01f, 0.0107405f, 0.01f}},
		{{14.1051f, 0.0f, 21.7484f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{7.88262f, 0.0f, 21.7484f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : candleData)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_candle", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize STATUE1
	constexpr castleData statue1[] = {
		{{3.04724f, 0.0f, -43.3982f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : statue1)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_statue1", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize STATUE2
	constexpr castleData statue2[] = {
		{{-3.42362f, 0.0f, -41.8548f}, {0.0f, 0.113092f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : statue2)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_statue2", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize STATUE3
	constexpr castleData statue3[] = {
		{{-14.1011f, 0.0f, -43.6773f}, {0.0f, 0.643292f, 0.0f}, {0.01f, 0.01f, 0.01f}},
		{{14.2448f, 0.0f, -43.6773f}, {0.0f, -0.927505f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : statue3)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_statue3", data.position, data.rotation, data.scale));
#pragma endregion

#pragma region Initialize THRONE
	constexpr castleData throne[] = {
		{{0.0f, 0.0f, -39.9693f}, {0.0f, 0.0f, 0.0f}, {0.01f, 0.01f, 0.01f}}
	};

	for (const auto& data : throne)
		AddGameObject(CreateStaticMesh(L"../FBXOutput/mesh_throne", data.position, data.rotation, data.scale));
#pragma endregion
}

void GameScene::CreateEffectSamples()
{
	struct EffectInfo {
		u16string name;
		float x;
		float y;
		float z;
	};

	vector<EffectInfo> info = {
		{u"Fireworks", 1.f, 0.f, -10.5f},
		{u"BloodLance", 1.f, 0.f, 0.5f},
		{u"Aura01_HDR2", 1.f, 0.f, 0.5f},
		{u"Benediction", 1.f, 10.f, -10.5f},
		{u"Atmosphere", 1.f, 10.f, -10.5f},
		{u"CandleFire4", 27.f, 29.f, -70.0f},
		{u"CandleFire4", -27.f, 29.f, -70.0f},
		{u"CandleFire3", 7.2f, 3.25f, -4.2f},
	};

	for (int i = 0; i < info.size(); ++i)
	{
		auto effectSample = make_shared<GameObject>();
		auto effectRenderer = effectSample->AddComponent<EffectRenderer>();
		auto transform = effectSample->AddComponent<Transform>();
		effectRenderer->Initialize(*coreRef);
		u16string path = u"../Effects/" + info[i].name + u".efk";
		effectRenderer->LoadEffect(path.c_str());
		transform->SetInitPosition(info[i].x, info[i].y, info[i].z);
		effectObjects.push_back(effectSample);
		AddGameObject(effectSample);
	}
}

shared_ptr<MainCharacter> GameScene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}

	return nullptr;
}

void GameScene::Release()
{
}

void GameScene::Reset()
{
	// TODO: 씬 데이터 리셋 코드 추가
	knightPool.clear();
	activePlayers.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	OutputDebugStringA("GameScene Data has been deleted!! \n----------------------------------------\n");
}

void GameScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void GameScene::HandlePacket(const Protocol::GamePacket& packet)
{
	const auto& header = packet.header();
	int sessionId = header.sessionid();

	switch (header.type()) {
		case Protocol::PacketType::SC_LOGIN: {
			OutputDebugStringA("SC_LOGIN packet received\n");
			Protocol::SC_LOGIN_PACKET login;
			if (login.ParseFromArray(packet.body().data(), packet.body().size())) {
				GET(Input).SetClientID(sessionId);
				OutputDebugStringA(("My Session ID: " + to_string(GET(Input).GetClientID()) + "\n").c_str());
			}
			break;
		}
		case Protocol::PacketType::SC_ADD: {
			OutputDebugStringA("SC_ADD packet received\n");
			Protocol::SC_ADD_PACKET add;
			if (add.ParseFromArray(packet.body().data(), packet.body().size())) {
				Protocol::Vec3 pos = add.pos();

				auto player = GetAvailableKnight();
				if (player) {
					player->SetId(sessionId);
					auto transform = player->GetComponent<Transform>();
					transform->SetInitPosition(pos.x(), pos.y(), pos.z());

					activePlayers[sessionId] = player;
				}

				if (sessionId == GET(Input).GetClientID()) {
					myPlayer = player;
					myPlayer->SetCamera(cam.get());
					OutputDebugStringA("My character activated!\n");
				} 
			}
			break;
		}
		case Protocol::PacketType::SC_MOVE_OBJECT: {
			Protocol::SC_MOVE_PACKET move;
			if (move.ParseFromArray(packet.body().data(), packet.body().size())) {
				Protocol::Vec3 pos = move.pos();

				auto it = activePlayers.find(sessionId);
				if (it != activePlayers.end())
				{
					auto transform = it->second->GetComponent<Transform>();
					transform->SetPosition(pos.x(), pos.y(), pos.z());
					transform->SetTargetRotation(move.rot());
				}
			}
			break; 
		}
		case Protocol::PacketType::SC_REMOVE: {
			OutputDebugStringA("SC_REMOVE packet received\n");
			break;
		}
		case Protocol::PacketType::SC_ATTACK: {
			Protocol::SC_ATTACK_PACKET attack;
			if (attack.ParseFromArray(packet.body().data(), packet.body().size())) {
				if (sessionId == GET(Input).GetClientID()) {
					// TODO : Client Attack Animation 보정
					OutputDebugStringA("SC_ATTACK_PACKET received\n");
				}
			}
			break;
		}
		case Protocol::PacketType::SC_DODGE: {
			Protocol::SC_DODGE_PACKET dodge;
			if (dodge.ParseFromArray(packet.body().data(), packet.body().size())) {
				if (sessionId == GET(Input).GetClientID()) {
					// TODO : Client Dodge Animation 보정
					OutputDebugStringA("SC_DODGE_PACKET received\n");
				}
			}
		}
	}
}

const float* GameScene::GetBackgroundColor()
{
	return Colors::Snow;
}

void GameScene::InitializeSceneObjectPools()
{
}

void GameScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nGameScene Data has been created!! \n");

	CreateKnightPool();

	CreateCastle();

	CreateEffectSamples();

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();
	 
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->ReleaseUploadBuffers();
	}
	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");

	SetNetworkManager(GET(Engine).GetNetworkManager());
	_nManager->Send(PacketFactory::CSLoginPacket());
	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void GameScene::UpdateScene(const float deltaTime)
{
	// 이펙트 OFF
	/*if (effectObjects.size() > 0 && GET(Input).GetKeyDown('1'))
		effectObjects[0]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 1 && GET(Input).GetKeyDown('2'))
		effectObjects[1]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 2 && GET(Input).GetKeyDown('3'))
		effectObjects[2]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 3 && GET(Input).GetKeyDown('4'))
		effectObjects[3]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 4 && GET(Input).GetKeyDown('5'))
		effectObjects[4]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 5 && GET(Input).GetKeyDown('6')) {
		effectObjects[5]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[6]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[7]->GetComponent<EffectRenderer>()->PlayEffect();
	}

	if (effectObjects[2] && myPlayer) {
		if (auto transform = effectObjects[2]->GetComponent<Transform>())
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			transform->SetInitPosition(pos.x, pos.y, pos.z);
		}
	}*/

	if (myPlayer)	// Temporary Code for Player Centered Shadow Mapping
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());
	}

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects);
}

void GameScene::RenderSceneDeferred()
{
	BoundingFrustum viewFrustum = cam->GetViewFrustum();
	//int objCount = 0;

	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
			if (!myPlayer && !obj->IsInFrustum(viewFrustum))
				continue;

			if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			{
				meshRenderer->RenderDeferred(*coreRef);
				//objCount++;

				// Render Hitbox
				//obj->RenderDebugBoundingBox(*coreRef, { 1, 0, 0, 1 });
			}
		}
	}

	//OutputDebugStringA(("Rendered objects count: " + to_string(objCount) + "\n").c_str());
}

void GameScene::RenderSceneForward()
{
	BoundingFrustum viewFrustum = cam->GetViewFrustum();

	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
			if (!myPlayer && !obj->IsInFrustum(viewFrustum))
				continue;

			if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
				meshRenderer->RenderForward(*coreRef);
		}
	}
}

void GameScene::RenderSceneShadow()
{
	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->RenderShadow(*coreRef);
		}
	}
}

void GameScene::RenderSceneEffects()
{
	BoundingFrustum viewFrustum = cam->GetViewFrustum();

	for (const auto& obj : gameObjects)
	{
		if (!obj->IsInFrustum(viewFrustum))
			continue;

		if (auto effectRenderer = obj->GetComponent<EffectRenderer>())
			effectRenderer->Render(*coreRef, cam.get());
	}
}

void GameScene::RequestSceneChange()
{
	/*if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Scene1);
	}*/
}
