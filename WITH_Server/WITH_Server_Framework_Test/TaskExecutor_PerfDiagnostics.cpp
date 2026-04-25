#include "pch.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionOps.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "TaskExecutor.h"
#include "WorldId.h"

namespace
{
    using Clock = std::chrono::steady_clock;

    constexpr uint64_t kNsPerUs = 1000;

    struct NodePerfMetric
    {
        std::atomic<int64_t> startNs{ -1 };
        std::atomic<int64_t> endNs{ -1 };
        std::atomic<uint64_t> threadHash{ 0 };
    };

    struct PerfRunContext
    {
        std::vector<NodePerfMetric>* metrics{ nullptr };
        uint64_t frameStartNs{ 0 };
        uint64_t workNs{ 0 };
        std::atomic<uint32_t> activeNodes{ 0 };
        std::atomic<uint32_t> maxActiveNodes{ 0 };
        std::atomic<uint32_t> executedNodes{ 0 };
    };

    struct PerfRig
    {
        ExecutionSourceRegistry sources;
        FrameTaskGraph graph;
        std::vector<ExecToken> tokens;
    };

    struct PerfCase
    {
        std::string name;
        uint32_t nodeCount{ 0 };
        uint64_t workUs{ 0 };
        bool chainDependencies{ false };
        uint32_t warmupIterations{ 0 };
        uint32_t measuredIterations{ 0 };
    };

    struct PerfRunStats
    {
        std::string caseName;
        uint32_t workers{ 0 };
        uint32_t iteration{ 0 };
        uint32_t nodeCount{ 0 };
        uint32_t edgeCount{ 0 };
        uint64_t workUs{ 0 };
        bool chainDependencies{ false };

        uint64_t wallNs{ 0 };
        uint64_t sumNodeNs{ 0 };
        uint64_t minNodeNs{ 0 };
        uint64_t maxNodeNs{ 0 };
        uint64_t idealNs{ 0 };
        int64_t overheadVsIdealNs{ 0 };
        double workerEfficiency{ 0.0 };
        uint32_t maxConcurrency{ 0 };
        uint32_t executedNodes{ 0 };
        uint32_t distinctThreads{ 0 };
    };

    PerfRunContext* g_perfRun{ nullptr };

    uint64_t NowNs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                Clock::now().time_since_epoch()).count());
    }

    double ToUs(uint64_t ns)
    {
        return static_cast<double>(ns) / static_cast<double>(kNsPerUs);
    }

    double ToUsSigned(int64_t ns)
    {
        return static_cast<double>(ns) / static_cast<double>(kNsPerUs);
    }

    void UpdateAtomicMax(std::atomic<uint32_t>& target, uint32_t value)
    {
        uint32_t current = target.load(std::memory_order_relaxed);
        while (current < value &&
               !target.compare_exchange_weak(
                   current,
                   value,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed))
        {
        }
    }

    void BusyWaitFor(uint64_t workNs)
    {
        if (workNs == 0)
            return;

        const uint64_t endNs = NowNs() + workNs;
        uint64_t spin = 0;
        while (NowNs() < endNs)
        {
            ++spin;
            if ((spin & 0x3ffu) == 0)
                std::atomic_signal_fence(std::memory_order_seq_cst);
        }
    }

    ExecCallResult PerfNodeFn(NodeExecContext& ctx)
    {
        PerfRunContext* run = g_perfRun;
        if (run == nullptr || run->metrics == nullptr)
            return ExecCallResult::Failed;

        const size_t nodeIndex = static_cast<size_t>(ctx.sourceToken - 1);
        if (nodeIndex >= run->metrics->size())
            return ExecCallResult::Failed;

        const uint32_t active =
            run->activeNodes.fetch_add(1, std::memory_order_acq_rel) + 1;
        UpdateAtomicMax(run->maxActiveNodes, active);

        NodePerfMetric& metric = (*run->metrics)[nodeIndex];
        const uint64_t startNs = NowNs();
        metric.startNs.store(
            static_cast<int64_t>(startNs - run->frameStartNs),
            std::memory_order_release);
        metric.threadHash.store(
            static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
            std::memory_order_release);

        BusyWaitFor(run->workNs);

        const uint64_t endNs = NowNs();
        metric.endNs.store(
            static_cast<int64_t>(endNs - run->frameStartNs),
            std::memory_order_release);

        run->executedNodes.fetch_add(1, std::memory_order_acq_rel);
        run->activeNodes.fetch_sub(1, std::memory_order_acq_rel);
        return ExecCallResult::Success;
    }

    ExecutionOps MakeDummyValidOps()
    {
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
    }

    void PackGraphEdges(
        FrameTaskGraph& graph,
        const std::vector<std::vector<ExecNodeId>>& preds,
        const std::vector<std::vector<ExecNodeId>>& succs)
    {
        graph.edges.clear();

        for (ExecNodeId nodeId = 0;
             nodeId < static_cast<ExecNodeId>(graph.nodes.size());
             ++nodeId)
        {
            ExecNodeRecord& node = graph.nodes[nodeId];

            node.predBegin = static_cast<uint32_t>(graph.edges.size());
            node.predCount = static_cast<uint32_t>(preds[nodeId].size());
            for (ExecNodeId pred : preds[nodeId])
                graph.edges.push_back(pred);

            node.succBegin = static_cast<uint32_t>(graph.edges.size());
            node.succCount = static_cast<uint32_t>(succs[nodeId].size());
            for (ExecNodeId succ : succs[nodeId])
                graph.edges.push_back(succ);
        }
    }

    void BuildPerfRig(const PerfCase& perfCase, PerfRig& rig)
    {
        rig.tokens.reserve(perfCase.nodeCount);
        rig.graph.nodes.reserve(perfCase.nodeCount);

        std::vector<std::vector<ExecNodeId>> preds(perfCase.nodeCount);
        std::vector<std::vector<ExecNodeId>> succs(perfCase.nodeCount);

        for (uint32_t i = 0; i < perfCase.nodeCount; ++i)
        {
            const ExecToken token = rig.sources.AllocateToken();
            rig.tokens.push_back(token);

            ExecutionSourceDesc desc{};
            desc.token = token;
            desc.phase = ExecPhase::Simulate;
            desc.lane = ExecLane::Parallel;
            desc.kind = ExecNodeKind::DynamicTask;
            desc.flags = ExecNodeFlag_None;
            desc.fn = &PerfNodeFn;
            desc.debugName = perfCase.name + ".node" + std::to_string(i);

            if (!rig.sources.Register(desc))
                throw std::runtime_error("failed to register perf source");

            ExecNodeRecord node{};
            node.id = i;
            node.scopeId = 0;
            node.phase = ExecPhase::Simulate;
            node.lane = ExecLane::Parallel;
            node.kind = ExecNodeKind::DynamicTask;
            node.flags = ExecNodeFlag_None;
            node.sourceToken = token;
            rig.graph.nodes.push_back(node);
        }

        if (perfCase.chainDependencies)
        {
            for (uint32_t i = 0; i + 1 < perfCase.nodeCount; ++i)
            {
                succs[i].push_back(i + 1);
                preds[i + 1].push_back(i);
            }
        }

        PackGraphEdges(rig.graph, preds, succs);

        rig.graph.scopeToWorld.push_back(1);
        rig.graph.scopeCount = 1;
        rig.graph.simulateNodeCount = perfCase.nodeCount;
    }

    PerfRunStats ExecuteMeasuredFrame(
        const PerfCase& perfCase,
        const PerfRig& rig,
        TaskExecutor& executor,
        uint32_t workers,
        uint32_t iteration,
        std::ofstream* nodeLog)
    {
        std::vector<NodePerfMetric> metrics(perfCase.nodeCount);
        std::vector<ExecNodeRuntime> nodeRuntime(rig.graph.nodes.size());
        std::vector<ExecScopeRuntime> scopeRuntime(rig.graph.scopeCount);

        ExecRuntimeState runtime{};
        runtime.BindViews(
            std::span<ExecNodeRuntime>(nodeRuntime.data(), nodeRuntime.size()),
            std::span<ExecScopeRuntime>(scopeRuntime.data(), scopeRuntime.size()),
            rig.graph.simulateNodeCount);

        ExecutionOps ops = MakeDummyValidOps();
        std::vector<WorldRuntime*> runtimeByScope(rig.graph.scopeCount, nullptr);
        std::vector<WorldId> worldIdByScope(rig.graph.scopeCount, WorldId::Create(900, 1));

        FrameExecContext frame{};
        frame.graph = &rig.graph;
        frame.ops = &ops;
        frame.runtimeByScope = runtimeByScope;
        frame.worldIdByScope = worldIdByScope;

        PerfRunContext run{};
        run.metrics = &metrics;
        run.workNs = perfCase.workUs * kNsPerUs;
        run.frameStartNs = NowNs();
        g_perfRun = &run;

        const uint64_t wallStartNs = run.frameStartNs;
        const bool ok = executor.ExecuteFrame(frame, runtime, rig.sources);
        const uint64_t wallEndNs = NowNs();

        g_perfRun = nullptr;

        if (!ok)
            throw std::runtime_error("TaskExecutor perf frame failed");

        PerfRunStats stats{};
        stats.caseName = perfCase.name;
        stats.workers = workers;
        stats.iteration = iteration;
        stats.nodeCount = perfCase.nodeCount;
        stats.edgeCount = static_cast<uint32_t>(rig.graph.edges.size() / 2);
        stats.workUs = perfCase.workUs;
        stats.chainDependencies = perfCase.chainDependencies;
        stats.wallNs = wallEndNs - wallStartNs;
        stats.minNodeNs = std::numeric_limits<uint64_t>::max();
        stats.maxConcurrency = run.maxActiveNodes.load(std::memory_order_acquire);
        stats.executedNodes = run.executedNodes.load(std::memory_order_acquire);

        std::unordered_set<uint64_t> threads;
        for (uint32_t i = 0; i < perfCase.nodeCount; ++i)
        {
            const int64_t startNs = metrics[i].startNs.load(std::memory_order_acquire);
            const int64_t endNs = metrics[i].endNs.load(std::memory_order_acquire);
            if (startNs < 0 || endNs < startNs)
                throw std::runtime_error("invalid perf node metric");

            const uint64_t durationNs = static_cast<uint64_t>(endNs - startNs);
            stats.sumNodeNs += durationNs;
            stats.minNodeNs = std::min(stats.minNodeNs, durationNs);
            stats.maxNodeNs = std::max(stats.maxNodeNs, durationNs);
            threads.insert(metrics[i].threadHash.load(std::memory_order_acquire));

            if (nodeLog != nullptr && nodeLog->is_open())
            {
                *nodeLog
                    << stats.caseName << ','
                    << stats.workers << ','
                    << stats.iteration << ','
                    << i << ','
                    << metrics[i].threadHash.load(std::memory_order_acquire) << ','
                    << ToUs(static_cast<uint64_t>(startNs)) << ','
                    << ToUs(static_cast<uint64_t>(endNs)) << ','
                    << ToUs(durationNs) << '\n';
            }
        }

        stats.distinctThreads = static_cast<uint32_t>(threads.size());

        if (perfCase.chainDependencies)
        {
            stats.idealNs = stats.sumNodeNs;
        }
        else
        {
            const uint64_t throughputIdeal =
                (stats.sumNodeNs + workers - 1) / workers;
            stats.idealNs = std::max(throughputIdeal, stats.maxNodeNs);
        }

        stats.overheadVsIdealNs =
            static_cast<int64_t>(stats.wallNs) - static_cast<int64_t>(stats.idealNs);

        const double capacityNs =
            static_cast<double>(stats.wallNs) * static_cast<double>(workers);
        stats.workerEfficiency =
            capacityNs > 0.0
                ? static_cast<double>(stats.sumNodeNs) / capacityNs
                : 0.0;

        if (stats.executedNodes != perfCase.nodeCount)
            throw std::runtime_error("perf run executed unexpected node count");

        return stats;
    }

    uint64_t MedianWallNs(std::vector<PerfRunStats> values)
    {
        if (values.empty())
            return 0;

        std::sort(
            values.begin(),
            values.end(),
            [](const PerfRunStats& lhs, const PerfRunStats& rhs)
            {
                return lhs.wallNs < rhs.wallNs;
            });

        return values[values.size() / 2].wallNs;
    }

    void WriteSummaryRow(
        std::ofstream& summaryLog,
        const PerfRunStats& stats,
        double speedupVsSerial)
    {
        summaryLog
            << stats.caseName << ','
            << stats.workers << ','
            << stats.iteration << ','
            << stats.nodeCount << ','
            << stats.edgeCount << ','
            << stats.workUs << ','
            << (stats.chainDependencies ? "chain" : "independent") << ','
            << ToUs(stats.wallNs) << ','
            << ToUs(stats.sumNodeNs) << ','
            << ToUs(stats.minNodeNs) << ','
            << ToUs(stats.maxNodeNs) << ','
            << ToUs(stats.idealNs) << ','
            << ToUsSigned(stats.overheadVsIdealNs) << ','
            << stats.workerEfficiency << ','
            << stats.maxConcurrency << ','
            << stats.distinctThreads << ','
            << stats.executedNodes << ','
            << speedupVsSerial << '\n';
    }

    void PrintCaseSummary(
        const PerfCase& perfCase,
        const std::vector<uint32_t>& workerCounts,
        const std::vector<PerfRunStats>& allStats)
    {
        std::vector<PerfRunStats> serialRuns;
        for (const PerfRunStats& stats : allStats)
        {
            if (stats.caseName == perfCase.name && stats.workers == 1)
                serialRuns.push_back(stats);
        }

        const uint64_t serialMedianNs = MedianWallNs(serialRuns);

        std::cout
            << "[PERF] case=" << perfCase.name
            << " nodes=" << perfCase.nodeCount
            << " workUs=" << perfCase.workUs
            << " shape=" << (perfCase.chainDependencies ? "chain" : "independent")
            << "\n";

        for (uint32_t workers : workerCounts)
        {
            std::vector<PerfRunStats> runs;
            for (const PerfRunStats& stats : allStats)
            {
                if (stats.caseName == perfCase.name && stats.workers == workers)
                    runs.push_back(stats);
            }

            if (runs.empty())
                continue;

            std::sort(
                runs.begin(),
                runs.end(),
                [](const PerfRunStats& lhs, const PerfRunStats& rhs)
                {
                    return lhs.wallNs < rhs.wallNs;
                });

            const PerfRunStats& medianRun = runs[runs.size() / 2];
            const uint64_t medianNs = medianRun.wallNs;
            const double speedup =
                medianNs > 0
                    ? static_cast<double>(serialMedianNs) / static_cast<double>(medianNs)
                    : 0.0;

            std::cout
                << "  workers=" << workers
                << " medianWallUs=" << std::fixed << std::setprecision(2) << ToUs(medianNs)
                << " speedupVs1=" << std::setprecision(2) << speedup
                << " maxConc~" << medianRun.maxConcurrency
                << " efficiency~" << std::setprecision(2) << medianRun.workerEfficiency
                << " overheadVsIdealUs~" << std::setprecision(2)
                << ToUsSigned(medianRun.overheadVsIdealNs)
                << "\n";
        }
    }
}

void RunTaskExecutorPerfDiagnostics()
{
    const std::vector<PerfCase> perfCases{
        { "independent_noop", 128, 0, false, 5, 30 },
        { "independent_200us", 64, 200, false, 3, 12 },
        { "chain_200us", 64, 200, true, 2, 8 },
    };

    const std::vector<uint32_t> workerCounts{ 1, 2, 4 };

    std::ofstream summaryLog{
        "WITH_Server_Framework_Test_PerfSummary.csv",
        std::ios::out | std::ios::trunc };
    std::ofstream nodeLog{
        "WITH_Server_Framework_Test_PerfNodes.csv",
        std::ios::out | std::ios::trunc };

    if (!summaryLog.is_open() || !nodeLog.is_open())
        throw std::runtime_error("failed to open perf log files");

    summaryLog
        << "case,workers,iteration,node_count,edge_count,work_us,shape,"
        << "wall_us,sum_node_us,min_node_us,max_node_us,ideal_us,"
        << "overhead_vs_ideal_us,worker_efficiency,max_concurrency,"
        << "distinct_threads,executed_nodes,speedup_vs_serial\n";

    nodeLog
        << "case,workers,iteration,node_index,thread_hash,"
        << "start_us,end_us,duration_us\n";

    std::vector<PerfRunStats> allStats;

    for (const PerfCase& perfCase : perfCases)
    {
        PerfRig rig{};
        BuildPerfRig(perfCase, rig);

        for (uint32_t workers : workerCounts)
        {
            TaskExecutor executor(workers);
            if (!executor.IsInitialized())
                throw std::runtime_error("failed to initialize perf TaskExecutor");

            for (uint32_t i = 0; i < perfCase.warmupIterations; ++i)
            {
                (void)ExecuteMeasuredFrame(
                    perfCase,
                    rig,
                    executor,
                    workers,
                    i,
                    nullptr);
            }

            std::vector<PerfRunStats> serialRunsForCase;
            for (uint32_t i = 0; i < perfCase.measuredIterations; ++i)
            {
                PerfRunStats stats = ExecuteMeasuredFrame(
                    perfCase,
                    rig,
                    executor,
                    workers,
                    i,
                    &nodeLog);

                allStats.push_back(stats);

                if (workers == 1)
                    serialRunsForCase.push_back(stats);
            }
        }

        std::vector<PerfRunStats> serialRuns;
        for (const PerfRunStats& stats : allStats)
        {
            if (stats.caseName == perfCase.name && stats.workers == 1)
                serialRuns.push_back(stats);
        }

        const uint64_t serialMedianNs = MedianWallNs(serialRuns);
        for (const PerfRunStats& stats : allStats)
        {
            if (stats.caseName != perfCase.name)
                continue;

            const double speedup =
                stats.wallNs > 0
                    ? static_cast<double>(serialMedianNs) / static_cast<double>(stats.wallNs)
                    : 0.0;
            WriteSummaryRow(summaryLog, stats, speedup);
        }

        PrintCaseSummary(perfCase, workerCounts, allStats);
    }

    std::cout
        << "[PERF] wrote WITH_Server_Framework_Test_PerfSummary.csv and "
        << "WITH_Server_Framework_Test_PerfNodes.csv\n";
}
