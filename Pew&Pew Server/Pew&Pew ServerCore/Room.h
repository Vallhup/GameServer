#pragma once

class Room {
public:
	Room() = delete;
	Room(int id, IGameContext& gameCtx) : _id(id), _gameCtx(gameCtx) {}

public:
	void AddCharacter(Character* character);
	void RemoveCharacter(int characterId);

	void AddProjectile(Projectile* projectile);
	void RemoveProjectile(int projectileId);

	bool IsFull() const { return _characters.size() == 2; }
	bool ContainsChar(int characterId) const;
	bool ContainsProj(int projectileId) const;
	std::vector<Character*> GetCharacterList() const { return _characters; };

	void OnDeath(int deathId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

private:
	IGameContext& _gameCtx;

	int _id;

	std::vector<Character*> _characters;
	std::vector<Projectile*> _projectiles;
};

