#include "pch.h"
#include "ClientTitleState.h"

void ClientTitleState::Clear()
{
	ownedTitleIds.clear();
	selectedIndex = 0;
	equippedTitleId = 0;
	lastError = 0;
	loaded = false;
	selectedNone = true;
	++revision;
}

void ClientTitleState::ApplyBootstrap(
	const Protocol::SC_STAT_UI_BOOTSTRAP_PACKET& packet)
{
	ownedTitleIds.assign(
		packet.ownedtitleids().begin(), packet.ownedtitleids().end());
	std::sort(ownedTitleIds.begin(), ownedTitleIds.end());
	ownedTitleIds.erase(
		std::unique(ownedTitleIds.begin(), ownedTitleIds.end()),
		ownedTitleIds.end());

	equippedTitleId = packet.equippedtitleid();
	if (equippedTitleId != 0 &&
		!std::binary_search(
			ownedTitleIds.begin(),
			ownedTitleIds.end(),
			equippedTitleId))
	{
		equippedTitleId = 0;
	}

	SelectTitle(equippedTitleId);
	lastError = 0;
	loaded = true;
	++revision;
}

void ClientTitleState::ApplyEquipResult(
	const Protocol::SC_TITLE_EQUIP_RESULT_PACKET& packet)
{
	lastError = packet.reason();
	if (packet.success())
	{
		equippedTitleId = packet.equippedtitleid();
	}

	SelectTitle(equippedTitleId);
	++revision;
}

bool ClientTitleState::SelectPrevious()
{
	return SelectOffset(-1);
}

bool ClientTitleState::SelectNext()
{
	return SelectOffset(1);
}

uint32_t ClientTitleState::GetSelectedTitleId() const
{
	return selectedNone || ownedTitleIds.empty()
		? 0
		: ownedTitleIds[selectedIndex];
}

bool ClientTitleState::SelectOffset(int offset)
{
	if (ownedTitleIds.empty())
	{
		return false;
	}

	if (selectedNone)
	{
		selectedIndex = offset < 0 ? ownedTitleIds.size() - 1 : 0;
		selectedNone = false;
		++revision;
		return true;
	}

	const size_t count = ownedTitleIds.size();
	if (offset < 0)
	{
		selectedIndex =
			selectedIndex == 0 ? count - 1 : selectedIndex - 1;
	}
	else
	{
		selectedIndex = (selectedIndex + 1) % count;
	}

	++revision;
	return true;
}

void ClientTitleState::SelectTitle(uint32_t titleId)
{
	selectedNone = titleId == 0;
	if (ownedTitleIds.empty())
	{
		selectedIndex = 0;
		return;
	}

	if (selectedNone)
	{
		selectedIndex = 0;
		return;
	}

	const auto it = std::find(
		ownedTitleIds.begin(),
		ownedTitleIds.end(),
		titleId);
	if (it == ownedTitleIds.end())
	{
		selectedIndex = 0;
		selectedNone = true;
		return;
	}

	selectedIndex = static_cast<size_t>(it - ownedTitleIds.begin());
}
