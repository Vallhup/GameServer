#pragma once
#include "Singleton.h"
#include "NetworkManager.h"

enum class MouseButton
{
	LEFT,
	RIGHT,

	END
};

class Input : public Singleton<Input>
{
	friend class Singleton<Input>;
	Input() = default;

public:
	void Renew();

	bool GetKey(const size_t key) const;
	bool GetKeyDown(const size_t key) const;
	bool GetAnyKeyDown() const;

	const XMFLOAT2& GetMousePosition() { return mMousePos; }
	bool GetMouseButton(const MouseButton button) const;
	bool GetMouseButtonDown(const MouseButton button) const;
	int GetMouseWheelDelta() const;

	void SetKey(const size_t key, const bool pressed);

	void SetMouseButton(const MouseButton button, const bool bPressed);
	void SetMousePosition(const XMFLOAT2 mousePosition);
	void SetMouseWheelDelta(int d);
	void SetClientID(int id);

	int GetClientID() const { return clientID; }

private:
	bitset<256> mPressedKeys = {};
	bitset<256> mChangeKeyState = {};

	XMFLOAT2 mMousePos = {};
	bool mPressedMouseButtons[static_cast<size_t>(MouseButton::END)] = {};
	bool mChangeMouseButtonState[static_cast<size_t>(MouseButton::END)] = {};
	int mMouseWheelDelta = 0;

	int clientID = -1;
};