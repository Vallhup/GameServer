#include "pch.h"
#include "RoomManager.h"

void RoomManager::AddCharacter(Character* character)
{
	for (auto& [id, room] : _rooms) {
		if (not room->IsFull()) {
			room->AddCharacter(character);
			return;
		}
	}

	auto newRoom = std::make_shared<Room>(_nextRoomId, _gameCtx);
	newRoom->AddCharacter(character);

	_rooms.try_emplace(_nextRoomId++, newRoom);
}

std::shared_ptr<Room> RoomManager::GetRoomByCharacter(int characterId)
{
	for (auto& [id, room] : _rooms) {
		if (room->ContainsChar(characterId)) return room;
	}

	return nullptr;
}

std::shared_ptr<Room> RoomManager::GetRoomByProjectile(int projectileId)
{
	for (auto& [id, room] : _rooms) {
		if (room->ContainsProj(projectileId)) return room;
	}

	return nullptr;
}
