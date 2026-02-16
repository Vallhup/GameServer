#pragma once

class TestWorldImpl final : public IWorldImpl {
public:
	explicit TestWorldImpl();


	virtual void SpawnInitial(WorldRuntime& rt) override;
	virtual Entity SpawnPlayer(WorldRuntime& rt, uint32 connId) override;

	virtual void Build(WorldRuntime& rt) override;

	virtual void Execute(WorldRuntime& rt, double dT) override;
	virtual void OnShutdown(WorldRuntime& rt) override;
};

