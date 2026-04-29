#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
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
    //
    // DynamicTask 등록 경로 전용: accesses span이 이 vector를 가리킨다.
    // Static System은 이 vector를 비워 둔다.
    // DynamicTaskTypeRegistry::Register()가 _types deque 내부 vector 주소를
    // span에 설정하므로, 이 필드는 사용되지 않는다. 단, 필요 시 직접 등록 경로에서
    // owned storage로 활용할 수 있다.
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

    // deque는 연속 메모리를 보장하지 않으므로 span 대신 range reference를 반환한다.
    // 빌더 내부의 range-based for 루프에서 직접 사용한다.
    [[nodiscard]]
    const std::deque<ExecutionSourceDesc>& GetSources() const noexcept
    {
        return _sources;
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
    //
    // deque 사용 이유:
    //   DynamicTaskTypeRegistry::Register()는 _types deque 내부의 accesses vector를
    //   가리키는 span을 ExecutionSourceDesc에 설정한 뒤 여기에 push_back한다.
    //   std::deque는 push_back 시 기존 원소의 주소를 무효화하지 않으므로
    //   이미 등록된 DynamicTask desc의 accesses span이 dangling되지 않는다.
    std::deque<ExecutionSourceDesc> _sources;

    // 1부터 시작 (0 = InvalidExecToken 예약)
    std::atomic<ExecToken> _nextToken{ 1 };
};
