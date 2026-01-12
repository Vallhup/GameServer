#include "pch.h"
#include "Input.h"

void Input::Renew()
{
	mChangeKeyState.reset();
	mMouseWheelDelta = 0;
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

int Input::GetMouseWheelDelta() const
{
	return mMouseWheelDelta;
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

void Input::SetMouseWheelDelta(int d)
{
	mMouseWheelDelta += d;
}

void Input::SetClientID(int id)
{
	OutputDebugStringA(("ClientId: " + to_string(id) + "\n").c_str());
	clientID = id;
}

void Input::SendMovePacket(int inputX, int inputZ, float yaw)
{
	if (!network) return;

	// TEMP : 나중에 별도로 헬퍼 함수 만들 수 있음
	Protocol::CS_MOVE_PACKET move;
	move.set_inputx(inputX);
	move.set_inputz(inputZ);
	move.set_yaw(yaw);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_MOVE_PACKET>(
		PacketType::CS_MOVE, move);
	network->Send(data);
}

void Input::SendAttackPacket()
{
	if (!network) return;

	Protocol::CS_ATTACK_PACKET attack;
	attack.set_dirx(0.0f);
	attack.set_dirz(0.0f);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_ATTACK_PACKET>(
		PacketType::CS_ATTACK, attack);
	network->Send(data);
}

void Input::SendDodgePacket()
{
	/*if (!network) return;

	vector<char> packet = PacketFactory::CSDodgePacket(clientID);
	network->Send(packet);*/
}
