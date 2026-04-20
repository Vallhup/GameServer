#pragma once

struct ClientInboundPacket;

class ClientPacketRouter {
public:
	void Route(const ClientInboundPacket& packet);

private:
	void RouteToCurrentScene(const ClientInboundPacket& packet);
	void HandleWorldTransitionBegin(const ClientInboundPacket& packet);
	void HandleWorldTransitionRejected(const ClientInboundPacket& packet);
};

