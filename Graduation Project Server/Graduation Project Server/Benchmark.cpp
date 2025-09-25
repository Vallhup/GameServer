#include "pch.h"
#include "Benchmark.h"

void Benchmark(const std::string& name, Function f)
{
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	f();
	auto end = high_resolution_clock::now();

	std::cout << "[ " << name << " ] : "
		<< duration_cast<milliseconds>(end - start).count() << "ms\n";
}

void ObjectPoolBenchmark()
{
	constexpr size_t loopCnt{ 1'000 };

	std::default_random_engine dre;
	std::uniform_int_distribution distAcquire(1, 1000);
	std::uniform_int_distribution distRelease(1, 1000);

	std::string str{ "ObjectPool Random IO Time" };
	Benchmark(str, [&]() {
		auto* pool = new ObjectPool<SendOver, 5000>;
		std::vector<SendOver*> active;
		active.reserve(5000);

		for (size_t i = 0; i < loopCnt; ++i) {
			int acq = distAcquire(dre);
			for (int j = 0; j < acq; ++j) {
				if (auto* obj = pool->Acquire()) {
					active.push_back(obj);
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
		});

	str = { "New/Delete Random IO Time" };
	Benchmark(str, [&]() {
		for (size_t i = 0; i < loopCnt; ++i) {
			std::vector<SendOver*> active;

			for (size_t i = 0; i < loopCnt; ++i) {
				// ·£´ý Acquire
				int acq = distAcquire(dre);
				for (int j = 0; j < acq; ++j) {
					active.push_back(new SendOver);
				}

				// ·£´ý Release
				int rel = distRelease(dre);
				for (int j = 0; j < rel && !active.empty(); ++j) {
					size_t idx = dre() % active.size();
					delete active[idx];
					active[idx] = active.back();
					active.pop_back();
				}
			}

			// ³²Àº active Á¤¸®
			for (auto* obj : active) {
				delete obj;
			}
		}
		});
}
