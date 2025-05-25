#include "pch.h"
#include "ViewListHelper.h"

bool ViewListHelper::can_see(const std::shared_ptr<GameObject> self, const std::shared_ptr<GameObject> target)
{
	if (abs(self->GetX() - target->GetX()) > VIEW_RANGE) return false;
	return abs(self->GetY() - target->GetY()) <= VIEW_RANGE;
}

std::unordered_set<int> ViewListHelper::collectViewList(const std::shared_ptr<GameObject> self, const std::shared_ptr<Service> service)
{
	if (nullptr == service) {
		return {};
	}

	// 1. self의 View Range가 걸치는 Sector에 있는 모든 client의 id 복사
	std::unordered_set<int> targetViewList = service->collectVisibleObjects(self);
	std::unordered_set<int> result;

	// 2. Sector 순회하면서 실제 self의 View Range안에 있는 client의 id Search
	for (int id : targetViewList) {
		if (id == self->GetId()) continue;

		auto it = service->_objects.find(id);
		if (it == service->_objects.end()) {
			return {};
		}

		GameObjectPtr target = it->second.load();
		if (nullptr == target) continue;
		if (not target->IsVisible()) continue;
		if (not target->IsAlive()) continue;
		if (can_see(self, target)) {
			result.insert(id);
		}
	}

	return result;
}

std::unordered_set<int> ViewListHelper::updateViewList(std::atomic<std::shared_ptr<std::unordered_set<int>>>& oldList, const std::unordered_set<int>& newList)
{
	std::unordered_set<int> prevList = *(oldList.load());
	oldList.store(std::make_shared<std::unordered_set<int>>(newList));
	return prevList;
}

ViewListDiff ViewListHelper::calcViewListDiff(const std::unordered_set<int>& oldList, const std::unordered_set<int>& newList)
{
	ViewListDiff result;

	// 1. 움직인 후 viewList에 있고
	for (int id : newList) {
		// 움직이기 전 viewList에는 없으면
		if (not oldList.count(id)) {
			// Add
			result.addViewList.push_back(id);
		}

		// 움직이기 전 viewList에도 있었으면
		else {
			// Move
			result.moveViewList.push_back(id);
		}
	}

	// 2. 움직이기 전 viewList에 있고
	for (int id : oldList) {
		// 움직인 후 viewList에는 없으면
		if (not newList.count(id)) {
			// Remove
			result.removeViewList.push_back(id);
		}
	}

	return result;
}