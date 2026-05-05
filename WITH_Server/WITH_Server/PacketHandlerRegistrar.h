#pragma once

#include "DynamicTaskTypes.h"

class DynamicTaskTypeRegistry;
class ExecutionSourceRegistry;
class NetworkRuntime;

// 서버 시작 시 한 번 호출.
// CS_ 패킷 타입별 DynamicTask ExecFn을 등록하고,
// PacketDispatchTable에 packetType → DynamicTaskTypeId 매핑을 채운다.
class PacketHandlerRegistrar final {
public:
	static void Register(
		DynamicTaskTypeRegistry& taskRegistry,
		ExecutionSourceRegistry& sourceRegistry,
		NetworkRuntime& network,
		DynamicTaskTypeId& outDisconnectedTypeId);
};
