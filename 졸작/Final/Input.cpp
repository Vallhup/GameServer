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

	SendInputPacket(key, pressed);
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

std::pair<Protocol::Input, Protocol::InputType> Input::GameInput(size_t key, bool pressed)
{
	WPARAM wParam = static_cast<WPARAM>(key);
	
	Protocol::Input input;
	switch (wParam) {
	case 'W': input = Protocol::Input::MOVE_FRONT; break;
	case 'A': input = Protocol::Input::MOVE_LEFT; break;
	case 'S': input = Protocol::Input::MOVE_BACK; break;
	case 'D': input = Protocol::Input::MOVE_RIGHT; break;
	}

	Protocol::InputType type = 
		pressed ? Protocol::InputType::KeyDown : Protocol::InputType::KeyUp;

	return { input, type };
}

void Input::SendInputPacket(const size_t key, const bool pressed)
{
	if (!network) return;

	auto input = GameInput(key, pressed);
	vector<char> packet = PacketFactory::CSInputPacket(input.first, input.second);
	network->Send(packet);
}
