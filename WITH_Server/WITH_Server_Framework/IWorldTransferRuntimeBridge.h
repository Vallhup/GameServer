#pragma once

#include <vector>
#include <cstdint>

#include "WorldId.h"

struct PlayerSnapshot;

class IWorldTransferRuntimeBridge {
public:
	virtual ~IWorldTransferRuntimeBridge() = default;

    virtual bool BuildTransferContext(
        WorldId sourceWorldId,
        const std::vector<uint32_t>& connectionIds,
        std::unique_ptr<ITransferContext>& outContext) = 0;

    virtual bool ImportTransferContext(
        WorldId targetWorldId,
        const ITransferContext& context,
        std::vector<uint32_t>& outImportedConnectionIds) = 0;

    virtual bool ReleaseTransferContext(
        WorldId sourceWorldId,
        const ITransferContext& context,
        std::vector<uint32_t>& outReleasedConnectionIds) = 0;

    virtual void RollbackImportedTransferContext(
        WorldId targetWorldId,
        const ITransferContext& context,
        const std::vector<uint32_t>& importedConnectionIds) = 0;
};