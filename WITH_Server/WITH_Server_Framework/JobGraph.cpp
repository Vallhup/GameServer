#include "pch.h"
#include "JobGraph.h"
#include "ThreadPool.h"
#include <ranges>
#include <fstream>

/*--------------------[ JobNode ]--------------------*/

void JobNode::AddDependency(JobNode* dependency)
{
	_deps.fetch_add(1);
	dependency->_dependents.push_back(this);
}

void JobNode::Execute()
{
	bool expected{ false };
	if (_done.compare_exchange_strong(expected, true)) 
	{
		_job.func(_job.context);

		for (JobNode* dep : _dependents)
		{
			if (dep->_deps.fetch_sub(1) == 1)
				_graph->Schedule(dep);
		}

		_graph->NotifyJobDone();
	}
}

/*--------------------[ JobGraph ]--------------------*/

JobGraph::~JobGraph()
{
	for (Job* job : _allocatedJobs)
		delete job;

	for (JobNode* node : _nodes)
		delete node;
}

void JobGraph::AutoDependencyBuild(std::span<System*> systems, double* dTRef)
{
	std::unordered_map<System*, JobNode*> nodeMap;
	nodeMap.reserve(systems.size());

	std::unordered_map<System*, int> stableOrder;
	stableOrder.reserve(systems.size());

	for (size_t i = 0; i < systems.size(); ++i)
	{
		System* sys = systems[i];

		JobNode* node = CreateNode<SystemJob>(sys, dTRef);
		node->_name = typeid(*sys).name();

		nodeMap[sys] = node;
		stableOrder[sys] = static_cast<int>(i);
	}

	for (size_t i = 0; i < systems.size(); ++i)
	{
		for (size_t j = i + 1; j < systems.size(); ++j)
		{
			System* A = systems[i];
			System* B = systems[j];

			DependencyType dep = AnalyzeDependency(A, B, stableOrder);
			if (dep == DependencyType::None) continue;

			System* first = nullptr;
			System* second = nullptr;

			if (dep == DependencyType::A_before_B)
			{
				first = A;
				second = B;
			}

			else if(dep == DependencyType::B_before_A)
			{
				first = B;
				second = A;
			}

			JobNode* from = nodeMap[first];
			JobNode* to = nodeMap[second];

			to->AddDependency(from);

#ifdef _DEBUG
			std::cout << "[Dependency] "
				<< typeid(*first).name() << " -> "
				<< typeid(*second).name() << "\n";
#endif

		}
	}

	//Build();
}

void JobGraph::AddManualDependency(System* before, System* after)
{
	auto beforeIt = std::find_if(_nodes.begin(), _nodes.end(), 
		[&](auto* n) 
		{
			auto* job = static_cast<SystemJob*>(n->_job.context);
			return job->system == before;
		});

	auto afterIt = std::find_if(_nodes.begin(), _nodes.end(), 
		[&](auto* n) 
		{
			auto* job = static_cast<SystemJob*>(n->_job.context);
			return job->system == after;
		});

	if (beforeIt != _nodes.end() && afterIt != _nodes.end())
		(*afterIt)->AddDependency(*beforeIt);
}

void JobGraph::Schedule(JobNode* node)
{
	JobData job{
		[](void* ctx) -> void
		{
			JobNode* n = static_cast<JobNode*>(ctx);
			n->Execute();
		}, node };

	_pool.Push(job);
}

void JobGraph::Build()
{
	for (JobNode* node : _nodes)
		node->_initDeps = node->_deps;

#ifdef _DEBUG
	DumpDot("graph.gv");
	for (JobNode* node : _nodes)
	{
		std::cout << "deps = " << node->_deps.load()
			<< (node->Ready() ? " [READY]" : " [WAIT]") << std::endl;
	}
#endif
}

void JobGraph::Run()
{
	new (&_latch) std::latch(static_cast<int>(_nodes.size()));

	for (JobNode* node : _nodes)
		node->ResetDeps();

	for (JobNode* node : _nodes)
	{
		if (node->Ready())
			Schedule(node);
	}

	_latch.wait();
}

void JobGraph::NotifyJobDone()
{
	_latch.count_down();
}

void JobGraph::DumpDot(std::string_view filename) const
{
	std::ofstream file(filename.data());
	file << "digraph JobGraph {\n";
	for (auto* node : _nodes)
	{
		for (auto* dep : node->Dependents())
			file << "  \"" << node->_name << "\" -> \"" << dep->_name << "\";\n";
		if (node->Dependents().empty())
			file << "  \"" << node->_name << "\";\n";
	}
	file << "}\n";
	file.close();

	std::cout << "[JobGraph] Dumped to " << filename << std::endl;
}