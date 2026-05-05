#pragma once

class IocpConnection;

// IocpNetworkBackend 가 새 TCP 연결을 수락할 때 게임 레이어에 통지하는 인터페이스.
// Framework 는 Session / SessionManager 를 직접 알지 않고,
// 이 인터페이스만 참조함으로써 계층 의존 방향(아래 → 위)을 유지한다.
//
// 구현체: NetworkRuntime (WITH_Server)
struct IConnectionAcceptSink
{
    virtual void OnConnectionAccepted(IocpConnection* connection) noexcept = 0;
    virtual ~IConnectionAcceptSink() = default;
};
