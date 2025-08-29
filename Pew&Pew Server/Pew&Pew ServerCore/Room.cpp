#include "pch.h"
#include "Room.h"

void Room::AddCharacter(Character* character)
{
	_characters.push_back(character);
}

bool Room::Contains(int characterId) const
{
	return std::any_of(_characters.begin(), _characters.end(),
		[&characterId](auto& c) { return c->GetId() == characterId; });
}

void Room::OnDeath(int deathId)
{
	for (auto& character : _characters) {
		// TODO : ½Â¸® Ã³¸®
	}
}
