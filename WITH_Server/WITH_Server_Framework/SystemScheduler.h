#pragma once

#include <cstdint>
#include <vector>
#include <span>

#include "SystemManager.h"

enum class DependencyOrder : uint8_t
{
	None,
	A_Before_B,
	B_Before_A
};

struct PairRule
{
	DependencyOrder order{ DependencyOrder::None };
	bool conflict{ false };
};

struct SystemBatch
{
	std::vector<System*> systems;
};

struct CompiledSystemSchedule
{
	std::vector<SystemBatch> batches;
};

class ThreadPool;

class SystemScheduler final {
public:
	CompiledSystemSchedule Compile(std::span<const SystemScheduleDesc> descs) const;
	void Execute(const CompiledSystemSchedule& schedule, ThreadPool& pool, const double dT);

private:
	PairRule ClassifyPair(const SystemScheduleDesc& a, const SystemScheduleDesc& b) const;

	static bool ContainsTag(std::span<const SystemTag> tags, SystemTag tag);
	static bool HasStructuralEffect(const SystemMeta& meta);

	static PairRule ClassifyAccess(const AccessSpec& a, const AccessSpec& b);
	static void MergeRule(PairRule& dst, const PairRule& src);

	static bool IsSnapshotRead(const AccessSpec& a);
	static bool IsImmediateWrite(const AccessSpec& a);
	static bool IsDeferredWrite(const AccessSpec& a);
	static bool IsDeferredEmit(const AccessSpec& a);
	static bool IsCommitConsume(const AccessSpec& a);
};

