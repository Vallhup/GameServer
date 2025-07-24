#pragma once

class ITimerManager {
public:
	virtual ~ITimerManager() = default;

public:
	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void AddRepeatedTask(const std::function<void(float)>& func, float intervalMs) = 0;
	virtual void AddOneTimeTask(const std::function<void()>& func, float delayMs) = 0;
};

class TimerManager : public ITimerManager {
	struct RepeatedTask {
		std::function<void(float)> func;
		std::chrono::milliseconds interval;
		std::chrono::high_resolution_clock::time_point nextExecTime;

		RepeatedTask(std::function<void(float)> f, std::chrono::milliseconds i)
			: func(f), interval(i), nextExecTime(std::chrono::high_resolution_clock::now() + interval) {}
	};

	struct OneTimeTask {
		std::function<void()> func;
		std::chrono::high_resolution_clock::time_point targetTime;

		bool operator<(const OneTimeTask& other) const
		{
			return targetTime > other.targetTime;
		}
	};

public:
	TimerManager();
	virtual ~TimerManager();

	TimerManager(const TimerManager&) = delete;
	TimerManager& operator=(const TimerManager&) = delete;

	TimerManager(TimerManager&&) = delete;
	TimerManager& operator=(TimerManager&&) = delete;

public:

	virtual void Start() override;
	virtual void Stop() override;

	virtual void AddRepeatedTask(const std::function<void(float)>& func, float intervalMs) override;
	virtual void AddOneTimeTask(const std::function<void()>& func, float delayMs) override;

private:
	void RepeatedTaskThreadLoop();
	void OneTimeTaskThreadLoop();

private:
	std::atomic<bool> _running;

	std::vector<RepeatedTask> _repeatedTasks;
	concurrency::concurrent_priority_queue<OneTimeTask> _oneTimeTaskQueue;

	std::thread _repeatedTaskThread;
	std::thread _oneTimeTaskThread;
};

