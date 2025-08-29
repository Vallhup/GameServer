#include "pch.h"
#include "RoomManager.h"

void RoomManager::AddCharacter(Character* character)
{
	if (_waiting) {
		_waiting = character;
	}

	else {
		auto room = std::make_shared<Room>(_nextRoomId, _gameCtx);
		room->AddCharacter(_waiting);
		room->AddCharacter(character);

		_rooms.try_emplace(_nextRoomId++, room);
		_waiting = nullptr;
	}
}

std::shared_ptr<Room> RoomManager::GetRoomByCharacter(int characterId)
{
	for (auto& [id, room] : _rooms) {
		if (room->Contains(characterId)) return room;
	}

	return nullptr;
}
