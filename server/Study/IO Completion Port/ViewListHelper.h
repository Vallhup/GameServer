#pragma once

struct ViewListDiff {
	std::vector<int> addViewList;
	std::vector<int> moveViewList;
	std::vector<int> removeViewList;
};

class ViewListHelper
{
public:
	static bool can_see(const std::shared_ptr<GameObject> self, const std::shared_ptr<GameObject> target);

	static std::unordered_set<int> collectViewList(const std::shared_ptr<GameObject> self, const std::shared_ptr<Service> service);
	static std::unordered_set<int> updateViewList(std::unordered_set<int>& oldList, const std::unordered_set<int>& newList);
	static ViewListDiff calcViewListDiff(const std::unordered_set<int>& oldList, const std::unordered_set<int>& newList);
};

