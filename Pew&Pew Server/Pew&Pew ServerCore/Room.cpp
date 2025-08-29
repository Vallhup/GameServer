#include "pch.h"
#include "Room.h"

void Room::AddCharacter(Character* character)
{
	_characters.push_back(character);
}

void Room::AddProjectile(Projectile* projectile)
{
	_projectiles.push_back(projectile);
}

void Room::RemoveProjectile(Projectile* projectile)
{
	std::remove(_projectiles.begin(), _projectiles.end(), projectile);
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
		const int charId = character->GetId();

		if (charId == deathId) {
			//sessMng.GetSession(charId)->Send(/* Lose Packet */);
		}

		else {
			//sessMng.GetSession(charId)->Send(/* Win  Packet */);
		}
	}
}