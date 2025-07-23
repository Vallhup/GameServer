#include "pch.h"
#include "CharacterManager.h"

void CharacterManager::AddCharacter(const std::shared_ptr<Character>& character)
{
	std::unique_lock lock{ _mutex };
	_characters.insert(std::make_pair(character->GetId(), character));
}

void CharacterManager::RemoveCharacter(int characterId)
{
	std::unique_lock lock{ _mutex };
	_characters.erase(characterId);
}

std::shared_ptr<Character> CharacterManager::GetCharacter(int characterid) const
{
	std::shared_lock lock{ _mutex };
	auto it = _characters.find(characterid);
	return (it != _characters.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Character>> CharacterManager::GetCharacterList() const
{
	std::shared_lock lock{ _mutex };

	std::vector<std::shared_ptr<Character>> characterList;
	characterList.reserve(_characters.size());

	for (const auto& [id, character] : _characters) {
		characterList.push_back(character);
	}

	return characterList;
}

void CharacterManager::Update(float deltaTime)
{
	for (auto& character : GetCharacterList()) {
		character->Update(deltaTime);
	}
}

std::vector<PendingProjectile> CharacterManager::GetPendingProjectiles(float nowTime)
{
	std::vector<PendingProjectile> result;

	for (auto& character : GetCharacterList()) {
		auto dirOpt = character->GetNextProjectile(nowTime);
		if (dirOpt.has_value()) {
			result.emplace_back(character->GetId(), character->GetPosition(), dirOpt.value());
		}
	}

	return result;
}

std::vector<std::shared_ptr<Character>> CharacterManager::GetDeathCharacterList() const
{
	std::shared_lock lock{ _mutex };

	std::vector<std::shared_ptr<Character>> characterList;
	characterList.reserve(_characters.size());

	for (const auto& [id, character] : _characters) {
		if (not character->IsAlive()) {
			characterList.push_back(character);
		}
	}

	return characterList;
}
