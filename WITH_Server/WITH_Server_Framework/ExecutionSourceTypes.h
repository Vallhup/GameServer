#pragma once

#include <atomic>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "ExecutionContextTypes.h"
#include "SystemMeta.h"

struct ExecutionSourceDesc
{
    ExecToken token{ InvalidExecToken };

    ExecPhase phase{ ExecPhase::None };
    ExecLane lane{ ExecLane::None };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };

    ExecFn fn{ nullptr };

    std::string debugName;

    // ECS write/read footprint — AutoSystemBridge가 SystemMeta.accesses로부터 채운다.
    // span은 소유권을 가지지 않으며, 정적 System의 경우 System 서브클래스의
    // static constexpr 배열을 가리킨다 (수명 = 프로세스).
    std::span<const AccessSpec> accesses{};

    // 스케줄링 품질 힌트 — AutoSystemBridge가 ExecMeta.schedulingHint로부터 복사한다.
    // ExecNodeRecord.priorityBias 로 전파되어 executor에서 사용된다.
    SchedulingHint schedulingHint{};

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            token != InvalidExecToken &&
            kind != ExecNodeKind::None &&
            fn != nullptr;
    }
};

class ExecutionSourceRegistry {
public:
    // 단조 증가 토큰을 발급한다. 0(= InvalidExecToken)은 건너뛴다.
    // AutoSystemBridge 및 모든 외부 등록 경로는 이 메서드를 통해서만 토큰을 취득해야 한다.
    [[nodiscard]]
    ExecToken AllocateToken() noexcept
    {
        return _nextToken.fetch_add(1, std::memory_order_relaxed);
    }

    // 레거시 하드코딩 토큰 경로 전용 — 원하는 값을 예약하고 중복 여부를 검사한다.
    [[nodiscard]]
    bool ReserveToken(ExecToken token)
    {
        if (token == InvalidExecToken)
            return false;

        if (TryGet(token) != nullptr)
            return false;

        // _nextToken이 예약값보다 낮으면 예약값 이후로 전진시켜 충돌을 방지한다.
        ExecToken expected = _nextToken.load(std::memory_order_relaxed);
        while (expected <= token)
        {
            if (_nextToken.compare_exchange_strong(
                    expected, token + 1,
                    std::memory_order_relaxed))
                break;
        }

        return true;
    }

    [[nodiscard]]
    const ExecutionSourceDesc* TryGet(ExecToken token) const noexcept
    {
        for (const ExecutionSourceDesc& desc : _sources)
        {
            if (desc.token == token)
                return &desc;
        }
        return nullptr;
    }

    [[nodiscard]]
    std::span<const ExecutionSourceDesc> GetSources() const noexcept
    {
        return std::span<const ExecutionSourceDesc>(_sources.data(), _sources.size());
    }

    [[nodiscard]]
    bool Register(const ExecutionSourceDesc& desc)
    {
        if (!desc.IsValid())
            return false;

        if (TryGet(desc.token) != nullptr)
            return false;

        _sources.push_back(desc);
        return true;
    }

    void Clear()
    {
        _sources.clear();
        _nextToken.store(1, std::memory_order_relaxed);
    }

private:
    // TODO: 선형 탐색 병목 시 unordered_map 기반으로 수정
    std::vector<ExecutionSourceDesc> _sources;

    // 1부터 시작 (0 = InvalidExecToken 예약)
    std::atomic<ExecToken> _nextToken{ 1 };
};
