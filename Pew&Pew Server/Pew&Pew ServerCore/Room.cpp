#include "pch.h"
#include "Room.h"

void Room::AddCharacter(Character* character)
{
	_characters.push_back(character);
}

void Room::RemoveCharacter(int characterId)
{
	std::erase_if(_characters,
		[&characterId](Character* c) { return c->GetId() == characterId; });
}

void Room::AddProjectile(Projectile* projectile)
{
	_projectiles.push_back(projectile);
}

void Room::RemoveProjectile(int projectileId)
{
	std::erase_if(_projectiles,
		[&projectileId](Projectile* p) { return p->GetId() == projectileId; });
}

bool Room::ContainsChar(int characterId) const
{
	return std::any_of(_characters.begin(), _characters.end(),
		[&characterId](auto& c) { return c->GetId() == characterId; });
}

bool Room::ContainsProj(int projectileId) const
{
	return std::any_of(_projectiles.begin(), _projectiles.end(),
		[&projectileId](auto& c) { return c->GetId() == projectileId; });
}

void Room::OnDeath(int deathId)
{
	auto& sessMng = _gameCtx.GetSessionManager();

	for (auto& character : _characters) {
		if (character) {
			const int charId = character->GetId();

			if (charId == deathId) {
				sessMng.GetSession(charId)->Send(PacketFactory::SCGameLosePacket());
			}
			else {
				sessMng.GetSession(charId)->Send(PacketFactory::SCGameWinPacket());
			}
		}
	}
}

void Room::BroadCast(const std::vector<char>& packet, int exceptId)
{
	auto& sessMng = _gameCtx.GetSessionManager();

	for (auto& character : _characters) {
		if (character) {
			if (character->GetId() == exceptId) continue;
			if (auto session = sessMng.GetSession(character->GetId())) {
				session->Send(packet);
			}
		}
	}
}