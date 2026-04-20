#include "pch.h"
#include "Input.h"

void Input::Renew()
{
	mChangeKeyState.reset();
	mMouseWheelDelta = 0;

	for (int i = 0; i < static_cast<int>(MouseButton::END); ++i)
		mChangeMouseButtonState[i] = false;
}

bool Input::GetKey(const size_t key) const
{
	return mPressedKeys[key];
}

bool Input::GetKeyDown(const size_t key) const
{
	return mPressedKeys[key] && mChangeKeyState[key];
}

bool Input::GetAnyKeyDown() const
{
	if ((mPressedKeys & mChangeKeyState).any())
		return true;

	if (GetMouseButton(MouseButton::LEFT) || GetMouseButton(MouseButton::RIGHT))
		return true;

	return false;
}

bool Input::GetMouseButton(const MouseButton button) const
{
	return mPressedMouseButtons[static_cast<size_t>(button)];
}

bool Input::GetMouseButtonDown(const MouseButton button) const
{
	size_t idx = static_cast<size_t>(button);
	return mPressedMouseButtons[idx] && mChangeMouseButtonState[idx];
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
	size_t idx = static_cast<size_t>(button);
	mChangeMouseButtonState[idx] = (mPressedMouseButtons[idx] != bPressed);
	mPressedMouseButtons[idx] = bPressed;
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