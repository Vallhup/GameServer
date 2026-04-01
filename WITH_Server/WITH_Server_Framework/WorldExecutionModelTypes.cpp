#include "pch.h"
#include "WorldExecutionModelTypes.h"

ExecPhase WorldExecutionModel::TryGetDeclaredPhase(ExecToken token) const noexcept
{
    if (token == InvalidExecToken)
        return ExecPhase::None;

    for (ExecToken t : simulateSources)
    {
        if (t == token)
            return ExecPhase::Simulate;
    }

    for (ExecToken t : commitSources)
    {
        if (t == token)
            return ExecPhase::Commit;
    }

    for (ExecToken t : lifecycleFlushSources)
    {
        if (t == token)
            return ExecPhase::LifecycleFlush;
    }

    for (ExecToken t : reconcileSources)
    {
        if (t == token)
            return ExecPhase::Reconcile;
    }

    return ExecPhase::None;
}

const WorldExecutionModel* WorldExecutionModelRegistry::TryGet(WorldExecutionModelKey key) const noexcept
{
    for (const WorldExecutionModel& model : _models)
    {
        if (model.key == key)
            return &model;
    }
    return nullptr;
}

bool WorldExecutionModelRegistry::Register(const WorldExecutionModel& model, const ExecutionSourceRegistry& sourceRegistry)
{
    if (!model.IsValid())
        return false;

    if (Has(model.key))
        return false;

    std::unordered_set<ExecToken> seenTokens;
    seenTokens.reserve(model.GetSourceCount());

    const auto validateBucket =
        [&](const std::vector<ExecToken>& tokens, const ExecPhase expectedPhase) -> bool
        {
            for (const ExecToken token : tokens)
            {
                if (token == InvalidExecToken)
                    return false;

                if (!seenTokens.insert(token).second)
                    return false;

                const ExecutionSourceDesc* desc = sourceRegistry.TryGet(token);
                if (desc == nullptr || !desc->IsValid())
                    return false;

                if (desc->phase != expectedPhase)
                    return false;
            }

            return true;
        };

    if (!validateBucket(model.simulateSources, ExecPhase::Simulate))
        return false;

    if (!validateBucket(model.commitSources, ExecPhase::Commit))
        return false;

    if (!validateBucket(model.lifecycleFlushSources, ExecPhase::LifecycleFlush))
        return false;

    if (!validateBucket(model.reconcileSources, ExecPhase::Reconcile))
        return false;

    for (const ExecutionDependencyEdge& edge : model.explicitEdges)
    {
        if (!edge.IsValid())
            return false;

        if (edge.fromToken == edge.toToken)
            return false;

        if (!model.ContainSourceToken(edge.fromToken) ||
            !model.ContainSourceToken(edge.toToken))
        {
            return false;
        }
    }

    _models.push_back(model);
    return true;
}
