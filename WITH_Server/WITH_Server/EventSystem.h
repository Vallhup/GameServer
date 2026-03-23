#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"

class EventSystem final : public System {
	using EventHandler = std::function<void(const Event&)>;

	static inline auto kMeta = MakeMetaStorage(
		SysTag<EventSystem>(),
		"EventSystem",
		std::array<AccessSpec, 0>{ }
	);

public:
	EventSystem(WorldRuntime& rt);

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ProcessConnect(const Event& event);
	void ProcessDisconnect(const Event& event);
	void ProcessMove(const Event& event);
	void ProcessAction(const Event& event);

	static XMFLOAT3 BuildMoveDirFromYawInput(
		const int inputX,
		const int inputZ,
		const float yaw
	);

	std::unordered_map<EventType, EventHandler> _handlers;
};