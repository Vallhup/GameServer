#include "pch.h"
#include "RoomManager.h"

void RoomManager::AddCharacter(Character* character)
{
	std::unique_lock lock{ _mutex };

	for (auto& [id, room] : _rooms) {
		if (not room->IsFull()) {
			room->AddCharacter(character);

			if (room->IsFull())
			{
				std::cout << "Room is full! Starting 10 second timer..." << std::endl;
				_gameCtx.GetTimerManager().AddOneTimeTask(
					[&room]()
					{
						std::cout << "10 seconds passed! Broadcasting game start..." << std::endl;
						room->BroadCast(PacketFactory::SCGameStartPacket());
					}, 10000.0f);
			}
			return;
		}
	}

	auto newRoom = std::make_shared<Room>(_nextRoomId, _gameCtx);
	newRoom->AddCharacter(character);

	_rooms.try_emplace(_nextRoomId++, newRoom);
}

void RoomManager::RemoveCharacter(int characterId)
{
	std::unique_lock lock{ _mutex };

	auto room = GetRoomByCharacter(characterId);
	room->RemoveCharacter(characterId);
}

std::shared_ptr<Room> RoomManager::GetRoomByCharacter(int characterId)
{
	std::shared_lock lock{ _mutex };

	for (auto& [id, room] : _rooms) {
		if (room->ContainsChar(characterId)) return room;
	}

	return nullptr;
}

std::shared_ptr<Room> RoomManager::GetRoomByProjectile(int projectileId)
{
	std::shared_lock lock{ _mutex };

	for (auto& [id, room] : _rooms) {
		if (room->ContainsProj(projectileId)) return room;
	}

	return nullptr;
}
