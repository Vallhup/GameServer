#pragma once

class TimerManager
{
public:
	TimerManager(int intervalMs = 4);
	~TimerManager();

	void Register(const std::function<void(float)>& tickFunc);
	void Start();
	void Stop();

private:
	void Run();

private:
	int _intervalMs;
	std::atomic<bool> _running;
	//concurrency::concurrent_vector<std::function<void(float)>> _tickFuncs;
	std::vector<std::function<void(float)>> _tickFuncs;
	std::thread _thread;
};