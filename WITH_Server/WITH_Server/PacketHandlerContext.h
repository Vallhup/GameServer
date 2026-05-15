#pragma once

class NetworkRuntime;
class FrameworkRuntime;
class SessionFlowController;
class CharacterDataService;
class CharacterSpawnService;
class IWorldTransitionRequestSink;
class ServerSessionSystem;
class ODBCDatabaseBackend;

// DynamicTask ExecFn은 raw function pointer라 클로저로 서비스를 캡처할 수 없다.
// 패킷 핸들러 ExecFn이 게임 서비스에 접근하기 위한 정적 서비스 로케이터.
// ServerApp::Initialize() 에서 네트워크 시작 전에 반드시 초기화해야 한다.
struct PacketHandlerContext
{
	NetworkRuntime*              network{ nullptr };
	FrameworkRuntime*            framework{ nullptr };
	SessionFlowController*       sessionFlow{ nullptr };
	CharacterDataService*        characterData{ nullptr };
	CharacterSpawnService*       characterSpawn{ nullptr };
	IWorldTransitionRequestSink* worldTransitionSink{ nullptr };
	ServerSessionSystem*         sessionSystem{ nullptr };
	ODBCDatabaseBackend*         database{ nullptr };

	static void Initialize(PacketHandlerContext& ctx) noexcept;
	static PacketHandlerContext& Get() noexcept;
};
