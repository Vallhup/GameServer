#include "pch.h"
#include "Inventory.h"

void Inventory::AddItem(char itemId, int count)
{
	if (count <= 0) {
		return;
	}

	auto it = _items.find(itemId);
	if (it != _items.end()) {
		it->second += count;
	}

	else {
		_items.emplace(itemId, count);
	}
}

bool Inventory::RemoveItem(char itemId, int count)
{
	if (count <= 0) {
		return false;
	}
	
	auto it = _items.find(itemId);
	if (it == _items.end()) {
		return false;
	}

	if (it->second <= count) {
		_items.erase(it);
	}

	else {
		it->second -= count;
	}

	return true;
}

int Inventory::getItemCount(char itemId) const
{
	return _items.at(itemId);
}

bool Inventory::hasItem(char itemId) const
{
	auto it = _items.find(itemId);
	return it != _items.end();
}
