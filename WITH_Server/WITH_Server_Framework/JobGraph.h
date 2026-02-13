#pragma once

#include <latch>
#include <optional>
#include <unordered_set>

#include "Job.h"

class ThreadPool;
class JobGraph;

class System;

struct JobData {
	void (*func)(void*);
	void* context;
};

class alignas(64) JobNode {
	friend class JobGraph;

public:
	explicit JobNode(const JobData& job, JobGraph* graph) 
		: _job(job), _graph(graph), _deps(0), _initDeps(0), 
		_done(false) {}

	void AddDependency(JobNode* dependency);
	void Execute();

	bool Ready() const { return _deps == 0; }
	void ResetDeps() { _deps = _initDeps; _done = false; }
	const std::vector<JobNode*>& Dependents() const { return _dependents; }
	
protected:
	JobData _job;
	JobGraph* _graph;
	std::vector<JobNode*> _dependents;
	std::atomic<int> _deps;
	int _initDeps;

	std::atomic<bool> _done;
	std::string _name;
};

template<typename T>
concept JobT = std::is_base_of_v<Job, T>;

class JobGraph {
	friend class JobNode;

public:
	explicit JobGraph(ThreadPool& pool) 
		: _pool(pool), _latch(0) {}
	~JobGraph();

	JobGraph(const JobGraph&) = delete;
	JobGraph& operator=(const JobGraph&) = delete;

	template<JobT T, typename... Args>
	JobNode* CreateNode(Args&&... args);

	void AutoDependencyBuild(std::span<System*> systems, float* dTRef);
	void AddManualDependency(System* before, System* after);

	void Build();
	void Run();

	const std::vector<Job*>& GetJobs() const { return _allocatedJobs; }

private:
	void Schedule(JobNode* node);
	void NotifyJobDone();

	void DumpDot(std::string_view fileName) const;

private:
	ThreadPool& _pool;
	std::vector<JobNode*> _nodes;
	std::vector<Job*> _allocatedJobs;
	std::latch _latch;
};

template<JobT T, typename... Args>
inline JobNode* JobGraph::CreateNode(Args&&... args)
{
	T* job = new T(std::forward<Args>(args)...);
	_allocatedJobs.push_back(job);

	JobData data{ T::Execute, job };
	JobNode* node = new JobNode(data, this);
	_nodes.push_back(node);

	return node;
}

/*--------------------[ Utility ]--------------------*/

enum class DependencyType {
	None,
	A_before_B,
	B_before_A
};

static inline bool Intersects(const std::vector<std::type_index>& a, const std::vector<std::type_index>& b)
{
	for (const std::type_index& x : a)
	{
		if (std::find(b.begin(), b.end(), x) != b.end())
			return true;
	}

	return false;
}

static inline DependencyType AnalyzeDependency(System* A, System* B, 
	const std::unordered_map<System*, int>& stableOrder)
{
	const auto& aR = A->ReadResources();
	const auto& aW = A->WriteResources();
	const auto& bR = B->ReadResources();
	const auto& bW = B->WriteResources();

	const bool aW_bW = Intersects(aW, bW);
	const bool aW_bR = Intersects(aW, bR);
	const bool aR_bW = Intersects(aR, bW);

	const bool need_order = aW_bW || (aW_bR && aR_bW);

	if (need_order)
	{
		// Priority가 작을수록 먼저 실행
		if (A->GetPriority() != B->GetPriority())
			return (A->GetPriority() < B->GetPriority())
				? DependencyType::A_before_B : DependencyType::B_before_A;

		// 동적 시스템 추가 예정 없어서 at으로 구현
		return (stableOrder.at(A) < stableOrder.at(B))
			? DependencyType::A_before_B : DependencyType::B_before_A;
	}

	if (aW_bR)
		return DependencyType::A_before_B;

	if (aR_bW)
		return DependencyType::B_before_A;

	return DependencyType::None;
}