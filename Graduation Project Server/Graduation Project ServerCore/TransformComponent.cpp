#include "pch.h"
#include "TransformComponent.h"

void TransformComponent::NetworkUpdate()
{
	if (VersionCheckAndChange()) {
		vec3 pos = GetPosition();

		Protocol::Vec3 protoPos;
		protoPos.set_x(pos.x);
		protoPos.set_y(pos.y);
		protoPos.set_z(pos.z);

		_instance->BroadCast(PacketFactory::SCMovePakcet(_owner.GetId(), protoPos));
		LOG_DBG("Send Move Packet");
	}
}