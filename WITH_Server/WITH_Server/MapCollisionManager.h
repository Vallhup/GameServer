#pragma once

#include "Collision.h"

enum class CharacterType : uint8 {
	Knight,
	Lancer,
};

inline uint8 ToInt(CharacterType type) { return static_cast<uint8>(type); }

struct CharacterMapCapsule {
	CapsuleView stand;
	CapsuleView dodge;
};

class MapCollisionManager {
public:
	static MapCollisionManager& Get()
	{
		static MapCollisionManager instance;
		return instance;
	}

	CharacterMapCapsule* GetCharacterCollider(CharacterType type);

	void LoadMapCollider(std::string_view path);
	void LoadCharacterCollider(CharacterType type, std::string_view path);

private:
	CharacterMapCapsule LoadCharacterColliderInternal(std::string_view path);

	std::array<std::unique_ptr<CharacterMapCapsule>, 3> _charMapCapsules;
};
