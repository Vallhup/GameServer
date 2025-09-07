#include "pch.h"
#include "Input.h"

#include "../../Graduation Project Server/Graduation Project ServerCore/PacketFactory.h"

Input& Input::Get()
{
	static Input input;
	return input;
}

void Input::Renew()
{
	mChangeKeyState.reset();
}

bool Input::GetKey(const size_t key) const
{
	return mPressedKeys[key];
}

bool Input::GetKeyDown(const size_t key) const
{
	return mPressedKeys[key] && mChangeKeyState[key];
}

bool Input::GetMouseButton(const MouseButton button) const
{
	return mPressedMouseButtons[static_cast<size_t>(button)];
}

void Input::SetKey(const size_t key, const bool pressed)
{
	mChangeKeyState[key] = (mPressedKeys[key] != pressed);
	mPressedKeys[key] = pressed;
}

void Input::SetMouseButton(const MouseButton button, const bool bPressed)
{
	size_t mouseidx = static_cast<size_t>(button);
	mPressedMouseButtons[mouseidx] = bPressed;
}

void Input::SetMousePosition(const XMFLOAT2 mousePosition)
{
	mMousePos = mousePosition;
}

void Input::SendMovePacket(XMFLOAT3 dir)
{
	if (!network) return;

	Protocol::Vec3 netDir;
	netDir.set_x(dir.x);
	netDir.set_y(dir.y);
	netDir.set_z(dir.z);

	vector<char> packet = PacketFactory::CSMovePacket(myId, netDir);
	network->Send(packet);
}
