#pragma once

class Inventory
{
public:
	Inventory(const std::shared_ptr<GameSession>& owner) : _owner(owner) {}

public:
	void AddItem(char itemId, int count);
	bool RemoveItem(char itemId, int count);
	
public:
	int getItemCount(char itemId) const;
	bool hasItem(char itemId) const;
	
private:
	std::unordered_map<char, int> _items;
	std::weak_ptr<GameSession> _owner;
};