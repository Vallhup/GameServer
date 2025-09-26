#pragma once

using Function = std::function<void()>;

void Benchmark(const std::string& name, Function f);

void ObjectPoolBenchmark();
void ObjectPoolBenchmarkMT();