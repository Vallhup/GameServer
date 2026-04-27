#include "pch.h"

#include <atomic>
#include <barrier>
#include <chrono>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "LFWSDeque.h"

/*---------------------------------------------------------------------------
 LFWSDeque vs MutexDeque 단독 벤치마크

 비교 대상: LFWSDeque<T, Capacity> vs std::deque<T> + std::mutex

 시나리오:
   1. Sequential      : 단일 스레드, owner만 Push/TryPop 반복
   2. ConcurrentDrain : owner가 배치 단위로 Push, owner + N thieves가 동시에 drain
                        (단일 deque를 모든 스레드가 공유)
   3. PerWorkerDrain  : 워커별 개별 deque 보유, dispatcher가 round-robin으로 Push
                        워커는 자신의 deque TryPop 우선, 빈 경우 다른 워커 deque TrySteal
                        (실제 TaskExecutor에 적용될 구조)

 측정 지표: median wall time (ns), ops/us
 출력: 콘솔 요약 + LFWSDeque_BenchMark.csv
---------------------------------------------------------------------------*/

namespace
{
    // -----------------------------------------------------------------------
    // 벤치마크 설정 상수
    // -----------------------------------------------------------------------

    constexpr int64_t  kDequeCapacity  = 1024;   // 2의 거듭제곱, DAG 최대 노드 수 기준
    constexpr uint32_t kSeqBatch       = 512;    // Sequential: 1라운드당 아이템 수
    constexpr uint32_t kSeqRounds      = 1000;   // Sequential: 총 라운드 수 (512,000 ops)
    constexpr uint32_t kDrainBatch     = 512;    // ConcurrentDrain: 1라운드당 아이템 수
    constexpr uint32_t kDrainRounds    = 400;    // ConcurrentDrain: 총 라운드 수
    constexpr uint32_t kWarmupRounds   = 5;
    constexpr uint32_t kMeasuredRounds = 15;

    using ItemType = uintptr_t;
    using Clock    = std::chrono::steady_clock;

    // -----------------------------------------------------------------------
    // MutexDeque: 비교 기준선
    // TryPop  = 후단(back) pop  → owner가 가장 최근에 넣은 아이템 (LFWSDeque와 동일)
    // TrySteal = 전단(front) pop → thief가 가장 오래된 아이템 (LFWSDeque와 동일)
    // -----------------------------------------------------------------------

    template<typename T>
    class MutexDeque
    {
    public:
        void Push(T item)
        {
            std::lock_guard<std::mutex> lock{ _mtx };
            _deque.push_back(item);
        }

        std::optional<T> TryPop()
        {
            std::lock_guard<std::mutex> lock{ _mtx };
            if (_deque.empty())
                return std::nullopt;
            T item = _deque.back();
            _deque.pop_back();
            return item;
        }

        std::optional<T> TrySteal()
        {
            std::lock_guard<std::mutex> lock{ _mtx };
            if (_deque.empty())
                return std::nullopt;
            T item = _deque.front();
            _deque.pop_front();
            return item;
        }

    private:
        std::deque<T>  _deque;
        std::mutex     _mtx;
    };

    template<typename TDeque>
    void PushDeque(TDeque& deque, ItemType item)
    {
        if constexpr (requires(TDeque& d, ItemType value) { d.TryPush(value); })
        {
            while (!deque.TryPush(item))
                std::this_thread::yield();
        }
        else
        {
            deque.Push(item);
        }
    }

    // -----------------------------------------------------------------------
    // 유틸리티
    // -----------------------------------------------------------------------

    uint64_t NowNs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                Clock::now().time_since_epoch()).count());
    }

    uint64_t CalcMedian(std::vector<uint64_t> samples)
    {
        std::sort(samples.begin(), samples.end());
        return samples[samples.size() / 2];
    }

    // 반복 측정 래퍼: 워밍업 후 kMeasuredRounds 중앙값 반환
    template<typename Fn>
    uint64_t MeasureMedianNs(Fn&& fn)
    {
        for (uint32_t i = 0; i < kWarmupRounds; ++i)
            (void)fn();

        std::vector<uint64_t> samples;
        samples.reserve(kMeasuredRounds);
        for (uint32_t i = 0; i < kMeasuredRounds; ++i)
            samples.push_back(fn());

        return CalcMedian(samples);
    }

    // -----------------------------------------------------------------------
    // 결과 행
    // -----------------------------------------------------------------------

    struct BenchRow
    {
        std::string scenario;
        std::string queueType;
        uint32_t    thiefCount{ 0 };
        uint64_t    totalOps{ 0 };
        uint64_t    medianNs{ 0 };
        double      opsPerUs{ 0.0 };
    };

    // -----------------------------------------------------------------------
    // Scenario 1: Sequential
    //   단일 스레드, Push kSeqBatch → TryPop kSeqBatch 를 kSeqRounds 반복
    //   락 오버헤드가 없는 상태에서의 순수 자료구조 비용 측정
    // -----------------------------------------------------------------------

    template<typename TDeque>
    uint64_t RunSequential(TDeque& deque)
    {
        const uint64_t start = NowNs();

        for (uint32_t round = 0; round < kSeqRounds; ++round)
        {
            for (uint32_t i = 0; i < kSeqBatch; ++i)
                PushDeque(deque, static_cast<ItemType>(i + 1));

            for (uint32_t i = 0; i < kSeqBatch; ++i)
                (void)deque.TryPop();
        }

        return NowNs() - start;
    }

    // -----------------------------------------------------------------------
    // Scenario 2: ConcurrentDrain
    //   - owner: 매 라운드 kDrainBatch개 Push, 이후 drain에 참여
    //   - N thieves: TrySteal 반복, 전체 소비 완료 시 종료
    //   - 라운드 간 경계: consumed가 해당 라운드 누적 목표치에 도달해야 다음 Push
    //
    //   실제 사용 패턴:
    //     TaskExecutor가 프레임 시작 시 DAG 노드를 ready queue에 push하고,
    //     워커들이 steal하며 동시에 실행하는 구조와 동일
    // -----------------------------------------------------------------------

    template<typename TDeque>
    uint64_t RunConcurrentDrain(TDeque& deque, uint32_t thiefCount)
    {
        const uint64_t kTotalItems = static_cast<uint64_t>(kDrainBatch) * kDrainRounds;

        alignas(64) std::atomic<uint64_t> consumed{ 0 };

        // thief 스레드: consumed가 전체 목표에 도달할 때까지 steal 반복
        std::vector<std::thread> thieves;
        thieves.reserve(thiefCount);
        for (uint32_t t = 0; t < thiefCount; ++t)
        {
            thieves.emplace_back([&]()
            {
                while (consumed.load(std::memory_order_relaxed) < kTotalItems)
                {
                    if (deque.TrySteal().has_value())
                        consumed.fetch_add(1, std::memory_order_relaxed);
                    else
                        std::this_thread::yield();
                }
            });
        }

        const uint64_t start = NowNs();

        // owner: 배치 push → drain 참여 → 다음 배치
        for (uint32_t round = 0; round < kDrainRounds; ++round)
        {
            for (uint32_t i = 0; i < kDrainBatch; ++i)
                PushDeque(deque, static_cast<ItemType>(i + 1));

            // 이번 라운드 누적 소비 목표
            const uint64_t targetConsumed =
                static_cast<uint64_t>(round + 1) * kDrainBatch;

            while (consumed.load(std::memory_order_relaxed) < targetConsumed)
            {
                if (deque.TryPop().has_value())
                    consumed.fetch_add(1, std::memory_order_relaxed);
                else
                    std::this_thread::yield();
            }
        }

        const uint64_t elapsed = NowNs() - start;

        for (std::thread& th : thieves)
            th.join();

        return elapsed;
    }

    // -----------------------------------------------------------------------
    // Scenario 3: PerWorkerDrain (워커별 분산 deque)
    //   - N 워커, 각자 고유 LFWSDeque 보유
    //   - 각 워커가 자신의 deque에 직접 Push (owner 스레드 = 해당 워커)
    //   - Push 완료 후 barrier로 동기화, drain 단계 진입
    //   - drain: 자신의 deque TryPop 우선, 빈 경우 다른 워커 deque TrySteal
    //
    //   [기존 오류 수정] dispatcher(메인 스레드)가 deque[i]에 Push하면서
    //   worker[i]가 동시에 TryPop하면 _bottom에 데이터 레이스 발생.
    //   Push/TryPop은 반드시 동일 owner 스레드에서만 호출해야 함.
    //
    //   ConcurrentDrain과의 핵심 차이:
    //     ConcurrentDrain → 모든 thief가 단일 _top에 CAS 경쟁
    //     PerWorkerDrain  → TryPop(CAS 없음) 우선, steal은 N개 _top에 분산
    // -----------------------------------------------------------------------

    uint64_t RunPerWorkerDrain(uint32_t workerCount)
    {
        // kDrainBatch(512)는 workerCount {2,4,8} 로 나누어 떨어짐
        const uint32_t kItemsPerWorkerPerRound = kDrainBatch / workerCount;
        const uint64_t kTotalItems =
            static_cast<uint64_t>(kItemsPerWorkerPerRound) * workerCount * kDrainRounds;

        using WorkerDeque = LFWSDeque<ItemType, kDequeCapacity>;
        std::vector<std::unique_ptr<WorkerDeque>> deques;
        deques.reserve(workerCount);
        for (uint32_t w = 0; w < workerCount; ++w)
            deques.push_back(std::make_unique<WorkerDeque>());

        std::vector<WorkerDeque*> dequePtr;
        dequePtr.reserve(workerCount);
        for (auto& d : deques)
            dequePtr.push_back(d.get());

        alignas(64) std::atomic<uint64_t> consumed{ 0 };
        alignas(64) std::atomic<uint32_t> readyCount{ 0 };
        alignas(64) std::atomic<bool>     startSignal{ false };

        // 라운드 경계: 모든 워커가 Push 완료 후 drain 단계로 일제히 진입
        std::barrier<> roundBarrier{ static_cast<std::ptrdiff_t>(workerCount) };

        std::vector<std::thread> workers;
        workers.reserve(workerCount);

        for (uint32_t myIdx = 0; myIdx < workerCount; ++myIdx)
        {
            workers.emplace_back([&, myIdx]()
            {
                readyCount.fetch_add(1, std::memory_order_relaxed);
                while (!startSignal.load(std::memory_order_acquire))
                    std::this_thread::yield();

                for (uint32_t round = 0; round < kDrainRounds; ++round)
                {
                    // 자신의 deque에 Push (owner 연산 → 안전)
                    for (uint32_t i = 0; i < kItemsPerWorkerPerRound; ++i)
                        PushDeque(*dequePtr[myIdx], static_cast<ItemType>(i + 1));

                    // 모든 워커 Push 완료 대기 후 drain 시작
                    roundBarrier.arrive_and_wait();

                    const uint64_t targetConsumed =
                        static_cast<uint64_t>(round + 1) * kDrainBatch;

                    while (consumed.load(std::memory_order_relaxed) < targetConsumed)
                    {
                        // 1. 자신의 deque TryPop (CAS 없음)
                        if (dequePtr[myIdx]->TryPop().has_value())
                        {
                            consumed.fetch_add(1, std::memory_order_relaxed);
                            continue;
                        }

                        // 2. 다른 워커 deque 순환 TrySteal (CAS, 단 N개에 분산)
                        bool stolen = false;
                        for (uint32_t v = 1; v < workerCount && !stolen; ++v)
                        {
                            const uint32_t victimIdx = (myIdx + v) % workerCount;
                            if (dequePtr[victimIdx]->TrySteal().has_value())
                            {
                                consumed.fetch_add(1, std::memory_order_relaxed);
                                stolen = true;
                            }
                        }

                        if (!stolen)
                            std::this_thread::yield();
                    }
                }
            });
        }

        while (readyCount.load(std::memory_order_relaxed) < workerCount)
            std::this_thread::yield();

        const uint64_t start = NowNs();
        startSignal.store(true, std::memory_order_release);

        for (std::thread& th : workers)
            th.join();

        return NowNs() - start;
    }

    // -----------------------------------------------------------------------
    // 결과 출력
    // -----------------------------------------------------------------------

    void PrintTable(const std::vector<BenchRow>& rows)
    {
        std::cout << "\n[BENCH] LFWSDeque vs MutexDeque\n";
        std::cout
            << std::left
            << std::setw(20) << "scenario"
            << std::setw(14) << "queue"
            << std::setw(10) << "thieves"
            << std::setw(16) << "total_ops"
            << std::setw(14) << "median_us"
            << std::setw(12) << "ops/us"
            << '\n';
        std::cout << std::string(86, '-') << '\n';

        for (const BenchRow& row : rows)
        {
            const double medianUs =
                static_cast<double>(row.medianNs) / 1000.0;

            std::cout
                << std::left
                << std::setw(20) << row.scenario
                << std::setw(14) << row.queueType
                << std::setw(10) << row.thiefCount
                << std::setw(16) << row.totalOps
                << std::fixed << std::setprecision(1)
                << std::setw(14) << medianUs
                << std::setw(12) << row.opsPerUs
                << '\n';
        }
    }

    void WriteCsv(const std::vector<BenchRow>& rows)
    {
        const std::string kCsvPath = "LFWSDeque_BenchMark.csv";
        std::ofstream csv{ kCsvPath, std::ios::out | std::ios::trunc };
        if (!csv.is_open())
            throw std::runtime_error("LFWSDeque_BenchMark.csv 파일 열기 실패");

        csv << "scenario,queue_type,thief_count,total_ops,median_ns,median_us,ops_per_us\n";

        for (const BenchRow& row : rows)
        {
            const double medianUs =
                static_cast<double>(row.medianNs) / 1000.0;

            csv
                << row.scenario    << ','
                << row.queueType   << ','
                << row.thiefCount  << ','
                << row.totalOps    << ','
                << row.medianNs    << ','
                << std::fixed << std::setprecision(2)
                << medianUs        << ','
                << row.opsPerUs    << '\n';
        }

        std::cout << "[BENCH] wrote " << kCsvPath << '\n';
    }

    // -----------------------------------------------------------------------
    // BenchRow 생성 헬퍼
    // -----------------------------------------------------------------------

    BenchRow MakeRow(
        const std::string& scenario,
        const std::string& queueType,
        uint32_t           thiefCount,
        uint64_t           totalOps,
        uint64_t           medianNs)
    {
        BenchRow row;
        row.scenario   = scenario;
        row.queueType  = queueType;
        row.thiefCount = thiefCount;
        row.totalOps   = totalOps;
        row.medianNs   = medianNs;
        row.opsPerUs   =
            medianNs > 0
                ? static_cast<double>(totalOps) /
                  (static_cast<double>(medianNs) / 1000.0)
                : 0.0;
        return row;
    }

} // namespace

// ---------------------------------------------------------------------------
// 외부 진입점
// ---------------------------------------------------------------------------

void RunLFWSDequeBenchmark()
{
    std::vector<BenchRow> rows;

    const std::vector<uint32_t> kThiefCounts{ 1, 2, 4, 8 };

    // =======================================================================
    // Scenario 1: Sequential
    // =======================================================================
    {
        constexpr uint64_t kTotalOps =
            static_cast<uint64_t>(kSeqBatch) * kSeqRounds * 2; // push + pop

        // LFWSDeque
        {
            const uint64_t medNs = MeasureMedianNs([&]()
            {
                LFWSDeque<ItemType, kDequeCapacity> deque;
                return RunSequential(deque);
            });

            rows.push_back(MakeRow("sequential", "LFWSDeque", 0, kTotalOps, medNs));
        }

        // MutexDeque
        {
            const uint64_t medNs = MeasureMedianNs([&]()
            {
                MutexDeque<ItemType> deque;
                return RunSequential(deque);
            });

            rows.push_back(MakeRow("sequential", "MutexDeque", 0, kTotalOps, medNs));
        }
    }

    // =======================================================================
    // Scenario 2: ConcurrentDrain (thieves 수 변화)
    // =======================================================================
    {
        constexpr uint64_t kTotalOps =
            static_cast<uint64_t>(kDrainBatch) * kDrainRounds;

        for (uint32_t thiefCount : kThiefCounts)
        {
            // LFWSDeque
            {
                const uint64_t medNs = MeasureMedianNs([&]()
                {
                    LFWSDeque<ItemType, kDequeCapacity> deque;
                    return RunConcurrentDrain(deque, thiefCount);
                });

                rows.push_back(
                    MakeRow("concurrent_drain", "LFWSDeque", thiefCount, kTotalOps, medNs));
            }

            // MutexDeque
            {
                const uint64_t medNs = MeasureMedianNs([&]()
                {
                    MutexDeque<ItemType> deque;
                    return RunConcurrentDrain(deque, thiefCount);
                });

                rows.push_back(
                    MakeRow("concurrent_drain", "MutexDeque", thiefCount, kTotalOps, medNs));
            }
        }
    }

    // =======================================================================
    // Scenario 3: PerWorkerDrain
    //   workerCount = 총 워커 수 (symmetric, owner/thief 구분 없음)
    //   ConcurrentDrain과 스레드 수 기준 비교:
    //     workerCount=2  ↔  ConcurrentDrain thiefCount=1  (총 2 스레드)
    //     workerCount=4  ↔  ConcurrentDrain thiefCount=3  (총 4 스레드, 미측정)
    //     workerCount=8  ↔  ConcurrentDrain thiefCount=7  (총 8 스레드, 미측정)
    // =======================================================================
    {
        constexpr uint64_t kTotalOps =
            static_cast<uint64_t>(kDrainBatch) * kDrainRounds;

        const std::vector<uint32_t> kWorkerCounts{ 2, 4, 8 };

        for (uint32_t workerCount : kWorkerCounts)
        {
            const uint64_t medNs = MeasureMedianNs([&]()
            {
                return RunPerWorkerDrain(workerCount);
            });

            rows.push_back(
                MakeRow("per_worker_drain", "LFWSDeque_PerWorker", workerCount, kTotalOps, medNs));
        }
    }

    // =======================================================================
    // 결과 출력
    // =======================================================================
    PrintTable(rows);
    WriteCsv(rows);
}
