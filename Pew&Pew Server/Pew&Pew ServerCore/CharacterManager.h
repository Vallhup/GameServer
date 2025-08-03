#pragma once

struct PendingProjectile {
	int characterId;
	vec3 position;
	vec3 direction;
};

class ICharacterManager {
public:
	virtual ~ICharacterManager() = default;

public:
	virtual void AddCharacter(const std::shared_ptr<Character>& character) = 0;
	virtual void RemoveCharacter(int characterId) = 0;

	virtual std::shared_ptr<Character> GetCharacter(int characterId) const = 0;
	virtual std::vector<std::shared_ptr<Character>> GetCharacterList() const = 0;

	virtual void Update(float deltaTime) = 0;

	virtual std::vector<PendingProjectile> GetPendingProjectiles(float nowTime) = 0;
	virtual std::vector<std::shared_ptr<Character>> GetDeathCharacterList() const = 0;
};

class CharacterManager : public ICharacterManager {
public:
	CharacterManager() = default;
	virtual ~CharacterManager() = default;

	CharacterManager(const CharacterManager&) = delete;
	CharacterManager& operator=(const CharacterManager&) = delete;
	
	CharacterManager(CharacterManager&&) = delete;
	CharacterManager& operator=(CharacterManager&&) = delete;

public:
	virtual void AddCharacter(const std::shared_ptr<Character>& character) override;
	virtual void RemoveCharacter(int characterId) override;

	virtual std::shared_ptr<Character> GetCharacter(int characterid) const override;
	virtual std::vector<std::shared_ptr<Character>> GetCharacterList() const override;

	virtual void Update(float deltaTime) override;

	virtual std::vector<PendingProjectile> GetPendingProjectiles(float nowTime) override;

	virtual std::vector<std::shared_ptr<Character>> GetDeathCharacterList() const override;

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Character>> _characters;
};