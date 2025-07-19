#pragma once

class TimerManager
{
	struct TimerTask {
		std::function<void(float)> func;
		float intervalMs;
		float elapsed;
	};

public:
	TimerManager();
	~TimerManager();

	void Register(const std::function<void(float)>& func, float intervalMs = 4.0f);
	void Start();
	void Stop();

private:
	void Run();

private:
	std::atomic<bool> _running;
	//concurrency::concurrent_vector<std::function<void(float)>> _tickFuncs;
	std::vector<TimerTask> _tasks;
	std::thread _thread;
};

float GetNowTime();