#include "pch.h"
#include "DynamicTaskScheduler.h"

void DynamicTaskScheduler::Submit(DynamicTaskRequest request) noexcept
{
    _pending.push(std::move(request));
}

void DynamicTaskScheduler::DrainInto(std::vector<DynamicTaskRequest>& out)
{
    DynamicTaskRequest req;
    while (_pending.try_pop(req))
        out.push_back(std::move(req));
}
