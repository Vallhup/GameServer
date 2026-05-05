#include "pch.h"
#include "DynamicTaskTypes.h"

#include "ExecutionSourceTypes.h"
#include "FrameworkLog.h"

namespace
{
    static constexpr const char* kLogCategory = "DynamicTask";
}

DynamicTaskTypeId DynamicTaskTypeRegistry::Register(
    DynamicTaskTypeDesc desc,
    ExecutionSourceRegistry& sourceRegistry)
{
    if (!desc.IsValid())
    {
        FWLOG_ERROR(kLogCategory,
            "DynamicTaskTypeRegistry::Register - desc is invalid (name='%s', fn=%p)",
            desc.debugName.c_str(), reinterpret_cast<void*>(desc.dispatchFn));
        return InvalidDynamicTaskTypeId;
    }

    // typeId / sourceToken 발급
    desc.typeId      = _nextId.fetch_add(1, std::memory_order_relaxed);
    desc.sourceToken = sourceRegistry.AllocateToken();

    // ExecutionSourceDesc 구성
    // accesses span은 _types deque 내부의 vector를 가리키게 된다.
    // deque는 push_back 시 기존 원소 주소를 보존하므로 안전하다.
    ExecutionSourceDesc sourceDesc;
    sourceDesc.token            = desc.sourceToken;
    sourceDesc.phase            = desc.defaultPhase;
    sourceDesc.lane             = desc.defaultLane;
    sourceDesc.kind             = ExecNodeKind::DynamicTask;
    sourceDesc.flags            = desc.flags;
    sourceDesc.fn               = desc.dispatchFn;
    sourceDesc.debugName        = desc.debugName;
    sourceDesc.schedulingHint   = desc.schedulingHint;
    // accesses는 _types에 push 후 원소 주소가 확정되므로 여기서 span을 설정하면 안 된다.
    // → _types에 push한 뒤 sourceDesc.accesses를 채운다.

    // _types에 먼저 push한다 (deque이므로 주소 안정)
    _types.push_back(std::move(desc));
    DynamicTaskTypeDesc& stored = _types.back();

    // 이제 stored.accesses의 주소가 확정됐으므로 span을 설정한다.
    sourceDesc.accesses = std::span<const AccessSpec>(
        stored.accesses.data(),
        stored.accesses.size());

    if (!sourceRegistry.Register(sourceDesc))
    {
        FWLOG_ERROR(kLogCategory,
            "DynamicTaskTypeRegistry::Register - ExecutionSourceRegistry::Register failed "
            "(typeId=%u, token=%u, name='%s')",
            stored.typeId, stored.sourceToken, stored.debugName.c_str());

        // 롤백: 방금 push한 타입을 제거하고 typeId를 무효화한다.
        _types.pop_back();
        return InvalidDynamicTaskTypeId;
    }

    FWLOG_DEBUG(kLogCategory,
        "DynamicTaskTypeRegistry::Register - OK (typeId=%u, token=%u, name='%s')",
        stored.typeId, stored.sourceToken, stored.debugName.c_str());

    return stored.typeId;
}
