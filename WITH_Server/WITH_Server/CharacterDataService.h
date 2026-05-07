#pragma once

#include <cstdint>

#include <DirectXMath.h>

#include "CharacterDef.h"
#include "Session.h"
#include "WorldId.h"

class FrameworkRuntime;

enum class CharacterDataResultCode : uint8_t
{
	Success,
	InvalidSession,
	InvalidCharacterId,
	CharacterDefNotFound,
	CharacterIdNotPlayable,
	StartupWorldUnavailable,
	WorldNotFound,
};

struct CharacterDataResult
{
	CharacterDataResultCode code{ CharacterDataResultCode::InvalidSession };
	SessionId sessionId{ 0 };
	CharacterId characterId{ CharacterId::None };
	WorldId worldId{ WorldId::Invalid() };
	const CharacterDef* characterDef{ nullptr };
	DirectX::XMFLOAT3 spawnPosition{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 spawnRotation{ 0.0f, 0.0f, 0.0f, 1.0f };

	bool Succeeded() const noexcept
	{
		return code == CharacterDataResultCode::Success;
	}
};

class CharacterDataService final {
public:
	struct Dependencies
	{
		FrameworkRuntime* framework{ nullptr };
		const WorldId* startupWorldId{ nullptr };
	};

public:
	explicit CharacterDataService(Dependencies deps = {});

	void SetDependencies(Dependencies deps) noexcept;

	CharacterDataResult ResolveCharacterSelect(
		SessionId sessionId,
		CharacterId characterId) const;

private:
	Dependencies _deps;
};
