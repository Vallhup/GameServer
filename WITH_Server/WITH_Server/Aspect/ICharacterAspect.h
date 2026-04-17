#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <DirectXMath.h>

#include "../CharacterDef.h"
#include "EntityId.h"
#include "NetId.h"
#include "Session.h"

class WorldRuntime;

struct CombatStatInitialState
{
	int32_t currentHp{ 0 };
	int32_t maxHp{ 0 };
	int32_t currentStamina{ 0 };
	int32_t maxStamina{ 0 };
	int32_t currentPoise{ 0 };
	int32_t maxPoise{ 0 };
	int32_t attackPower{ 0 };
	int32_t defense{ 0 };
	float attackSpeed{ 1.0f };
	float moveSpeed{ 2.5f };
};

// 캐릭터 조립 시 호출자가 제공하는 외부 입력값.
// (CharacterDef 에는 없는, 인스턴스마다 달라지는 값들)
struct AssembleParams
{
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	NetId netId;

	// Playable 캐릭터 전용
	std::optional<SessionId> sessionId;

	std::optional<CombatStatInitialState> combatStatsOverride;
};

// 캐릭터 런타임의 한 가지 기능 단위(수직 슬라이스).
// 각 Aspect 는 자기 자신의:
//   - storage 등록
//   - entity 부착
//   - Def 기반 검증
//   - (장기) 전이 직렬화
// 를 한 객체에 모은다.
class ICharacterAspect {
public:
	virtual ~ICharacterAspect() = default;

	// 이 aspect 가 어떤 feature 를 요구하는가.
	// None 이면 모든 캐릭터에 적용된다.
	virtual CharacterFeatureFlags RequiredFeature() const noexcept = 0;

	// 부팅 시 1회 호출. 이 aspect 가 다루는 컴포넌트 storage 등록.
	virtual void RegisterStorages(WorldRuntime& runtime) const = 0;

	// 스폰 시 호출. entity 에 컴포넌트 부착 + Def/params 기반 초기화.
	virtual void Attach(
		WorldRuntime& runtime,
		Entity entity,
		const CharacterDef& def,
		const AssembleParams& params) const = 0;

	// 부팅 검증. 사전조건이 def 에서 충족되는지.
	virtual bool Validate(
		const CharacterDef& def,
		std::string& outError) const
	{
		(void)def;
		(void)outError;
		return true;
	}
};
