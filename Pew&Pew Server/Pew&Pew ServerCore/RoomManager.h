#pragma once

class IRoomManager {
public:
	virtual ~IRoomManager() = default;

public:
	virtual void AddCharacter(Character* character) = 0;
	virtual std::shared_ptr<Room> GetRoomByCharacter(int characterId) = 0;
};

class RoomManager : public IRoomManager {
public:
	RoomManager() = delete;
	RoomManager(IGameContext& gameCtx)
		: _gameCtx(gameCtx), _nextRoomId(0), _waiting(nullptr) {
	}

public:
	virtual void AddCharacter(Character* character) override;
	virtual std::shared_ptr<Room> GetRoomByCharacter(int characterId) override;

private:
	IGameContext& _gameCtx;

	int _nextRoomId;

	Character* _waiting;
	std::unordered_map<int, std::shared_ptr<Room>> _rooms;
};