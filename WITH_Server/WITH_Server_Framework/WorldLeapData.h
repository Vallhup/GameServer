#pragma once

#include "types.h"
#include "WorldId.h"
#include "WorldDesc.h"

enum class WorldLeapResult : uint8
{
	Success,
	StaleFromWorld,
	ToWorldDead,
	SnapshotFail,
	SpawnFail,
	ApplyFail,
	WorldCreateFail,
};

enum LeapMode : uint8
{
	Default,
	RecoverToSquare
};

struct WorldLeapRequest
{
	std::array<uint32, 3> connIds;
	WorldId fromWorldId;
	WorldType toWorldType;

	// 나중에 party 같은거 들어오면 partyId로
	// 0이면 무조건 World 생성 / 0 아니면 Resolve(없으면 생성)
	uint64 instanceKey{ 0 }; 
	LeapMode mode;
};

// WorldType
// 
// 1. Squre
// 2. Start
// 3. Middle
// 4. Final
// 5. PVP
//
// 위의 World 중 Squre만 계속 살려놓고 나머지 World들은 Request에 따라 1회성으로 생성, 삭제
// WorldLeapRequest를 처리할 때 Squre이면 이미 만들어져있는 World로 이동하고 나머지 Type은 항상 생성하도록