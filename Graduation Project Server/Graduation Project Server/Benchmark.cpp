#include "pch.h"
#include "Benchmark.h"

void Benchmark(const std::string& name, Function f)
{
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	f();
	auto end = high_resolution_clock::now();

	std::cout << "[ " << name << " ] : "
		<< duration_cast<milliseconds>(end - start).count() << "ms\n\n";
}

void ObjectPoolBenchmark()
{
	constexpr size_t loopCnt{ 100'000 };

	std::default_random_engine dre;
	std::uniform_int_distribution distAcquire(1, 100);
	std::uniform_int_distribution distRelease(1, 100);

	std::string str{ "ObjectPool Random IO Time" };
	Benchmark(str, [&]() {
		int realLoopCnt{ 0 };
		int nullCnt{ 0 };

		auto* pool = new ObjectPool<SendOver, 5000>;
		std::vector<SendOver*> active;
		active.reserve(5000);

		for (size_t i = 0; i < loopCnt; ++i) {
			int acq = distAcquire(dre);
			for (int j = 0; j < acq; ++j) {
				if (auto* obj = pool->Acquire()) {
					active.push_back(obj);
					realLoopCnt++;
				}

				else {
					realLoopCnt++;
					nullCnt++;
				}
			}

			int rel = distRelease(dre);
			for (int j = 0; j < rel and not active.empty(); ++j) {
				size_t idx = dre() % active.size();
				pool->Release(active[idx]);
				active[idx] = active.back();
				active.pop_back();
			}
		}

		for (auto* obj : active) {
			pool->Release(obj);
		}

		delete pool;

		std::cout << "Real Loop Count : " << realLoopCnt << std::endl;
		std::cout << "nullptr Count : " << nullCnt << std::endl;
		});

	ObjectPoolBenchmarkMT();

	str = { "New/Delete Random IO Time" };
	Benchmark(str, [&]() {
		int realLoopCnt{ 0 };
		std::vector<SendOver*> active;

		for (size_t i = 0; i < loopCnt; ++i) {
			int acq = distAcquire(dre);
			for (int j = 0; j < acq; ++j) {
				active.push_back(new SendOver);
				realLoopCnt++;
			}

			int rel = distRelease(dre);
			for (int j = 0; j < rel && !active.empty(); ++j) {
				size_t idx = dre() % active.size();
				delete active[idx];
				active[idx] = active.back();
				active.pop_back();
			}
		}

		for (auto* obj : active) {
			delete obj;
		}
		
		std::cout << "Real Loop Count : " << realLoopCnt << std::endl;
		});
}

void ObjectPoolBenchmarkMT()
{
	constexpr size_t loopCnt{ 100'000 };
	const size_t threadCount{ 4 };

	std::atomic<int> realLoopCnt{ 0 };
	std::atomic<int> nullCnt{ 0 };

	std::uniform_int_distribution distAcquire(1, 100);
	std::uniform_int_distribution distRelease(1, 100);

	std::string str{ "ObjectPool MT Random IO Time" };
	Benchmark(str, [&]() {
		auto worker = [&]() {
			thread_local ObjectPool<SendOver, 2500> pool;

			int localLoopCnt{ 0 };
			int localNullCnt{ 0 };

			std::default_random_engine localDre;
			std::vector<SendOver*> active;
			active.reserve(2000);

			for (size_t i = 0; i < loopCnt / threadCount; ++i) {
				int acq = distAcquire(localDre);
				for (int j = 0; j < acq; ++j) {
					if (auto* obj = pool.Acquire()) {
						active.push_back(obj);
						localLoopCnt++;
					}

					else {
						localLoopCnt++;
						localNullCnt++;
					}
				}

				int rel = distRelease(localDre);
				for (int j = 0; j < rel and not active.empty(); ++j) {
					size_t idx = localDre() % active.size();
					pool.Release(active[idx]);
					active[idx] = active.back();
					active.pop_back();
				}
			}

			for (auto* obj : active) {
				pool.Release(obj);
			}

			realLoopCnt += localLoopCnt;
			nullCnt += localNullCnt;
			};

		std::vector<std::thread> threads;
		for (size_t t = 0; t < threadCount; ++t) {
			threads.emplace_back(worker);
		}

		for (auto& th : threads) {
			th.join();
		}

		std::cout << "Real Loop Count : " << realLoopCnt << std::endl;
		std::cout << "nullptr Count : " << nullCnt << std::endl;
		});
}
