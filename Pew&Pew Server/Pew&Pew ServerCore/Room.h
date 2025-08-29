#pragma once

class Room {
public:
	Room() = delete;
	Room(int id, IGameContext& gameCtx) : _id(id), _gameCtx(gameCtx) {}

public:
	void AddCharacter(Character* character);

	bool Contains(int characterId) const;

	void OnDeath(int deathId);

private:
	IGameContext& _gameCtx;

	int _id;
	std::vector<Character*> _characters;
};

