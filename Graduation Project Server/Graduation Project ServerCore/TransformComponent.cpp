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

		_instance->BroadCast(PacketFactory::SCMovePakcet(_owner.GetId(), protoPos, _angle));
		//LOG_DBG("Send Move Packet");
	}
}

void TransformComponent::Translate(const vec3& delta)
{
	_pos += delta; 
	_angle = atan2f(-delta.x, -delta.z);
	++_version;
}