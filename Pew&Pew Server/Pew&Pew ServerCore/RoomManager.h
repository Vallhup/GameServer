#pragma once

class IRoomManager {
public:
	virtual ~IRoomManager() = default;

public:
	virtual void AddCharacter(Character* character) = 0;
	virtual void RemoveCharacter(int characterId) = 0;

	virtual std::shared_ptr<Room> GetRoomByCharacter(int characterId) = 0;
	virtual std::shared_ptr<Room> GetRoomByProjectile(int projectileId) = 0;
};

class RoomManager : public IRoomManager {
public:
	RoomManager() = delete;
	RoomManager(IGameContext& gameCtx)
		: _gameCtx(gameCtx), _nextRoomId(0) {}

public:
	virtual void AddCharacter(Character* character) override;
	virtual void RemoveCharacter(int characterId) override;

	virtual std::shared_ptr<Room> GetRoomByCharacter(int characterId) override;
	virtual std::shared_ptr<Room> GetRoomByProjectile(int projectileId) override;

private:
	IGameContext& _gameCtx;

	int _nextRoomId;

	std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Room>> _rooms;
};