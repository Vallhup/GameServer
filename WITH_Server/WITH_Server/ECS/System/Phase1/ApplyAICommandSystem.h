#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ApplyAICommandSystem final : public System{
    static const StaticSystemMetaStorage<3> kMetaStorage;

public:
    void Execute(SystemContext& ctx) override;
    const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
