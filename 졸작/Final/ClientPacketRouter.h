#pragma once

struct ClientInboundPacket;

class ClientPacketRouter {
public:
	void Route(const ClientInboundPacket& packet);

private:
	void RouteToCurrentScene(const ClientInboundPacket& packet);
	void HandleWorldTransitionBegin(const ClientInboundPacket& packet);
	void HandleWorldTransitionRejected(const ClientInboundPacket& packet);
	void HandlePartyUiBootstrap(const ClientInboundPacket& packet);
	void HandlePartyListSnapshot(const ClientInboundPacket& packet);
	void HandlePartyCommandResult(const ClientInboundPacket& packet);
	void HandlePartySnapshot(const ClientInboundPacket& packet);
	void HandlePartyJoinRequestReceived(const ClientInboundPacket& packet);
	void HandlePartyJoinRequestClosed(const ClientInboundPacket& packet);
};
