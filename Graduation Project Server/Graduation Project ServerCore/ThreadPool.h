#pragma once

class ThreadPool {
public:
	ThreadPool() = default;
	~ThreadPool() { Stop(); }

public:
	void Start(size_t threadCount);
	void Start(size_t threadCount, std::function<void()> func);
	void Stop();

	void Enqueue(std::function<void()> task);

private:
	void WorkerLoop();

private:
	std::atomic<bool> _running;
	std::vector<std::thread> _workers;

	std::mutex _mutex;
	std::condition_variable _cv;
	std::queue<std::function<void()>> _tasks;
};

