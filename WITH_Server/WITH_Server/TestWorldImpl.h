#pragma once

class TestWorldImpl final : public IWorldImpl {
public:
	explicit TestWorldImpl();

	virtual void Configure(WorldBuilder& builder) override;

	virtual Entity SpawnPlayer(WorldRuntime& rt, uint32 connId) override;

	virtual void OnShutdown(WorldRuntime& rt) override;
};

