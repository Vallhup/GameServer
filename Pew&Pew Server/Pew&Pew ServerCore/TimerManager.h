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
	void RegisterOnce(const std::function<void()>& func, float delayMs = 5000.0f);
	void Start();
	void Stop();

private:
	void Run();

private:
	std::atomic<bool> _running;
	concurrency::concurrent_vector<TimerTask> _tasks;
	/*std::vector<TimerTask> _tasks;
	std::mutex _taskMutex;*/
	std::thread _thread;
};

float GetNowTime();