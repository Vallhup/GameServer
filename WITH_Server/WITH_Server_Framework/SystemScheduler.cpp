#include "pch.h"
#include "SystemScheduler.h"
#include "ThreadPool.h"

CompiledSystemSchedule SystemScheduler::Compile(std::span<const SystemScheduleDesc> descs) const
{
	CompiledSystemSchedule schedule;

	const size_t n = descs.size();
	if (n == 0) return schedule;

	std::vector<std::vector<int>> outs(n);
	std::vector<int> indegree(n, 0);
	std::vector<std::vector<bool>> conflict(n, std::vector<bool>(n, false));

	for (int i = 0; i < n; ++i)
	{
		for (int j = i + 1; j < n; ++j)
		{
			const PairRule rule = ClassifyPair(descs[i], descs[j]);
			switch (rule.order) {
			case DependencyOrder::A_Before_B:
			{
				outs[i].push_back(j);
				++indegree[j];
				break;
			}
			case DependencyOrder::B_Before_A:
			{
				outs[j].push_back(i);
				++indegree[i];
				break;
			}
			case DependencyOrder::None:
			{
				break;
			}
			}

			if (rule.order == DependencyOrder::None && rule.conflict)
			{
				conflict[i][j] = true;
				conflict[j][i] = true;
			}
		}
	}

	std::vector<bool> scheduled(n, false);
	int remaining{ static_cast<int>(n) };

	while (remaining > 0)
	{
		std::vector<int> ready;
		ready.reserve(n);

		for (int i = 0; i < n; ++i)
		{
			if (!scheduled[i] && indegree[i] == 0)
				ready.push_back(i);
		}

		if (ready.empty())
			throw std::runtime_error("Hard dependency cycle detected in system metadata.");

		std::sort(ready.begin(), ready.end(),
			[&descs](int lhs, int rhs)
			{
				return descs[lhs].registrationOrder < descs[rhs].registrationOrder;
			});

		SystemBatch batch;
		std::vector<int> picked;
		picked.reserve(ready.size());

		for (int index : ready)
		{
			bool ok{ true };
			for (int other : picked)
			{
				if (conflict[index][other])
				{
					ok = false;
					break;
				}
			}

			if (!ok) continue;

			picked.push_back(index);
			batch.systems.push_back(descs[index].system);
		}

		if (batch.systems.empty())
		{
			const int index = ready.front();
			picked.push_back(index);
			batch.systems.push_back(descs[index].system);
		}

		schedule.batches.push_back(std::move(batch));

		for (int index : picked)
		{
			scheduled[index] = true;
			--remaining;
		}

		for (int index : picked)
		{
			for (int next : outs[index])
				--indegree[next];
		}
	}

	return schedule;
}

namespace
{
	struct SystemTaskCtx
	{
		System* system{ nullptr };
		double dT{ 0.0 };
	};

	void ExecuteSystemTask(void* p)
	{
		if (auto* ctx = static_cast<SystemTaskCtx*>(p))
		{
			if (ctx->system)
				ctx->system->Execute(ctx->dT);
		}
	}
}

void SystemScheduler::Execute(const CompiledSystemSchedule& schedule, ThreadPool& pool, const double dT)
{
	for (const SystemBatch& batch : schedule.batches)
	{
		if (batch.systems.empty())
			continue;

		const size_t count = batch.systems.size();
		if (count == 1)
		{
			if (System* system = batch.systems[0])
			{
				system->Execute(dT);
			}

			else
			{
				throw std::runtime_error("Null system found in compiled schedule.");
			}

			continue;
		}

		TaskCounter counter;
		std::vector<SystemTaskCtx> ctxs(count);

		size_t submitted{ 0 };
		bool submitFailed{ false };

		for (size_t i = 0; i < count; ++i)
		{
			System* system = batch.systems[i];
			if (!system)
			{
				submitFailed = true;
				break;
			}

			ctxs[i].system = system;
			ctxs[i].dT = dT;

			if (!pool.Submit(&ExecuteSystemTask, &ctxs[i], counter))
			{
				submitFailed = true;
				break;
			}

			++submitted;
		}

		std::exception_ptr batchException{ nullptr };

		try
		{
			if (submitted > 0)
				pool.Wait(counter);
		}

		catch (const std::logic_error& e)
		{
			batchException = std::current_exception();
			std::cout << e.what() << std::endl;
		}

		if (batchException)
		{
			std::rethrow_exception(batchException);
		}
			

		if (submitFailed)
			throw std::runtime_error("ThreadPool::Submit failed while executing a system batch.");
	}
}

PairRule SystemScheduler::ClassifyPair(const SystemScheduleDesc& a, const SystemScheduleDesc& b) const
{
	PairRule out;

	const SystemMeta& aMeta = *a.meta;
	const SystemMeta& bMeta = *b.meta;

	const bool aBeforeB =
		ContainsTag(aMeta.runsBefore, bMeta.tag) ||
		ContainsTag(bMeta.runsAfter,  aMeta.tag);

	const bool bBeforeA = 
		ContainsTag(bMeta.runsBefore, aMeta.tag) ||
		ContainsTag(aMeta.runsAfter,  bMeta.tag);

	if (aBeforeB && bBeforeA)
		throw std::runtime_error("Mutually contradictory runsBefore / runsAfter metadata.");

	if (aBeforeB) out.order = DependencyOrder::A_Before_B;
	if (bBeforeA) out.order = DependencyOrder::B_Before_A;

	if (HasStructuralEffect(aMeta) || HasStructuralEffect(bMeta))
		out.conflict = true;

	for (const AccessSpec& aAccess : aMeta.accesses)
	{
		for (const AccessSpec& bAccess : bMeta.accesses)
		{
			PairRule r = ClassifyAccess(aAccess, bAccess);
			MergeRule(out, r);
		}
	}

	return out;
}

bool SystemScheduler::ContainsTag(std::span<const SystemTag> tags, SystemTag tag)
{
	return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool SystemScheduler::HasStructuralEffect(const SystemMeta& meta)
{
	for (const AccessSpec& a : meta.accesses)
	{
		if (a.effect != StructuralEffect::None)
			return true;
	}

	return false;
}

PairRule SystemScheduler::ClassifyAccess(const AccessSpec& a, const AccessSpec& b)
{
	PairRule out;

	if (a.resource != b.resource)
		return out;

	// 1. 둘 다 Snapshot Read면 완전 독립
	if (IsSnapshotRead(a) && IsSnapshotRead(b))
		return out;

	// 2. Snapshor Read vs Deferred Write는 
	//    같은 프레임 Current Resource를 건드리지 않으므로 독립으로 판정
	if ((IsSnapshotRead(a) && IsDeferredWrite(b)) ||
		(IsDeferredWrite(a) && IsSnapshotRead(b)))
	{
		return out;
	}

	// 3. Deferred Emit -> Commit Consume은 명확한 Producer -> Consumer
	if (IsDeferredEmit(a) && IsCommitConsume(b))
	{
		out.order = DependencyOrder::A_Before_B;
		return out;
	}

	if (IsCommitConsume(a) && IsDeferredEmit(b))
	{
		out.order = DependencyOrder::B_Before_A;
		return out;
	}

	// 4. 나머지는 보수적으로 Conflict
	out.conflict = true;
	return out;
}

void SystemScheduler::MergeRule(PairRule& dst, const PairRule& src)
{
	if (src.order != DependencyOrder::None)
	{
		if (dst.order == DependencyOrder::None)
		{
			dst.order = src.order;
		}

		else if (dst.order != src.order)
		{
			throw std::runtime_error("Conflicting Dependency-order metadata detected.");
		}
	}

	dst.conflict = dst.conflict || src.conflict;

	// Dependency Order가 생기면 같은 Batch에 들어갈 수 없으므로
	// Conflit 유지 필요 X
	if (dst.order != DependencyOrder::None)
		dst.conflict = false;
}

bool SystemScheduler::IsSnapshotRead(const AccessSpec& a)
{
	return
		a.mode == AccessMode::Read &&
		a.visibility == Visibility::Snapshot;
}

bool SystemScheduler::IsImmediateWrite(const AccessSpec& a)
{
	return
		a.mode == AccessMode::Write &&
		a.visibility == Visibility::Immediate;
}

bool SystemScheduler::IsDeferredWrite(const AccessSpec& a)
{
	return
		a.mode == AccessMode::Write &&
		a.visibility == Visibility::Deferred;
}

bool SystemScheduler::IsDeferredEmit(const AccessSpec& a)
{
	return
		a.mode == AccessMode::Emit &&
		a.visibility == Visibility::Deferred;
}

bool SystemScheduler::IsCommitConsume(const AccessSpec& a)
{
	return
		a.mode == AccessMode::Consume &&
		a.visibility == Visibility::Commit;
}
