#pragma once

#include "BTNode.h"

class Instance;

class Job {
public:
	Job(std::chrono::high_resolution_clock::time_point time
	= std::chrono::high_resolution_clock::now()) : targetTime(time) {}
	virtual ~Job() = default;

public:
	virtual void Execute() = 0;

	bool operator>(const Job& other)
	{
		return targetTime < other.targetTime;
	}

protected:
	std::chrono::high_resolution_clock::time_point targetTime;
};

class TimerJob : public Job {
public:
	TimerJob() = delete;
	TimerJob(const std::function<void()>& f, std::chrono::high_resolution_clock::time_point time);
	virtual ~TimerJob() = default;

public:
	virtual void Execute() override;

private:
	std::function<void()> func;
};

class BTJob : public Job {
public:
	BTJob() = delete;
	BTJob(const std::function<NodeStatus()>& f);
	virtual ~BTJob() = default;

public:
	virtual void Execute() override;

private:
	std::function<NodeStatus()> func;
	std::shared_ptr<std::promise<NodeStatus>> promise;
};

class LogicJob : public Job {
public:
	LogicJob() = delete;
	LogicJob(const std::shared_ptr<Instance>& i, float dT);
	virtual ~LogicJob() = default;

public:
	virtual void Execute() override;

private:
	std::weak_ptr<Instance> instance;
	float deltaTime;
};


//class ObjectBatchJob : public Job {
//public:
//	ObjectBatchJob(Instance* instance, float deltaTime, size_t start, size_t end)
//		: _instance(instance), _deltaTime(deltaTime), _start(start), _end(end) {
//	}
//
//	virtual void Execute() override;
//
//private:
//	Instance* _instance;
//	float _deltaTime;
//	size_t _start;
//	size_t _end;
//};
//
//
//class FinalizeJob : public Job {
//public:
//	explicit FinalizeJob(Instance* inst) : _inst(inst) {}
//	void Execute() override;
//private:
//	Instance* _inst;
//};