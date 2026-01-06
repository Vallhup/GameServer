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

	void AutoDependencyBuild(const std::vector<System*>& systems, float* dTRef);
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

	JobData data{ T::Execute, job};
	JobNode* node = new JobNode(data, this);
	_nodes.push_back(node);

	return node;
}

/*--------------------[ Utility ]--------------------*/

static inline bool Intersects(const std::vector<std::type_index>& a, const std::vector<std::type_index>& b)
{
	for (const std::type_index& x : a)
	{
		if (std::find(b.begin(), b.end(), x) != b.end())
			return true;
	}

	return false;
}

static inline bool HasConflict(System* A, System* B)
{
	const auto& aR = A->ReadComponents();
	const auto& aW = A->WriteComponents();
	const auto& bR = B->ReadComponents();
	const auto& bW = B->WriteComponents();

	if (Intersects(aW, bR) ||
		Intersects(aW, bW) ||
		Intersects(bW, aR))
		return true;

	return false;
}