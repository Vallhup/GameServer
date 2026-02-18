#pragma once

// 세션별 상태 복제

struct SessionRepState
{
	uint32 sessId{ 0 };
	std::unordered_set<uint64> knownIds;
};

