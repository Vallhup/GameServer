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
	TimerJob(int id, const std::function<void()>& f, std::chrono::high_resolution_clock::time_point time);
	virtual ~TimerJob() = default;

public:
	virtual void Execute() override;

private:
	int objectId;
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
	LogicJob(Instance* i, float dT);
	virtual ~LogicJob() = default;

public:
	virtual void Execute() override;

private:
	Instance* instance;
	float deltaTime;
};

class DBJob : public Job {
public:
	DBJob() = default;
	virtual ~DBJob() = default;

public:
	virtual void Execute() override;

private:

};