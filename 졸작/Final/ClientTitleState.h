#pragma once

class ClientTitleState
{
public:
	void Clear();

	void ApplyBootstrap(
		const Protocol::SC_STAT_UI_BOOTSTRAP_PACKET& packet);
	void ApplyEquipResult(
		const Protocol::SC_TITLE_EQUIP_RESULT_PACKET& packet);

	bool SelectPrevious();
	bool SelectNext();

	const std::vector<uint32_t>& GetOwnedTitleIds() const
	{
		return ownedTitleIds;
	}

	uint32_t GetEquippedTitleId() const { return equippedTitleId; }
	uint32_t GetSelectedTitleId() const;

	uint64_t GetRevision() const { return revision; }
	bool IsLoaded() const { return loaded; }

private:
	bool SelectOffset(int offset);
	void SelectTitle(uint32_t titleId);

	std::vector<uint32_t> ownedTitleIds;
	size_t selectedIndex{ 0 };
	uint32_t equippedTitleId{ 0 };
	uint32_t lastError{ 0 };
	uint64_t revision{ 0 };
	bool loaded{ false };
	bool selectedNone{ true };
};
