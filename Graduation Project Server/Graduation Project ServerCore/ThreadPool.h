#pragma once

class ThreadPool {
public:
	ThreadPool() = default;
	~ThreadPool() { Stop(); }

public:
	void Start(size_t threadCount, std::function<void()> func);
	void Stop();

private:
	std::atomic<bool> _running;
	std::vector<std::thread> _workers;
};

