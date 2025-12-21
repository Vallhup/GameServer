#pragma once

#include "NetworkManager.h"

enum class MouseButton
{
	LEFT,
	RIGHT,

	END
};

class Input final
{
public:
	static void Initialize(NetworkManager* net) { Get().network = net; }
	static Input& Get();

	void Renew();

	bool GetKey(const size_t key) const;
	bool GetKeyDown(const size_t key) const;

	const XMFLOAT2& GetMousePosition() { return mMousePos; }
	bool GetMouseButton(const MouseButton button) const;
	int GetMouseWheelDelta() const;

	void SetKey(const size_t key, const bool pressed);

	void SetMouseButton(const MouseButton button, const bool bPressed);
	void SetMousePosition(const XMFLOAT2 mousePosition);
	void SetMouseWheelDelta(int d);
	void SetClientID(int id);

	NetworkManager* GetNetworkManager() const { return network; }
	int GetClientID() const { return clientID; }

public:
	void SendMovePacket(int inputX, int intputZ, float yaw);
	void SendAttackPacket();
	void SendDodgePacket();

private:
	bitset<256> mPressedKeys = {};
	bitset<256> mChangeKeyState = {};

	XMFLOAT2 mMousePos = {};
	bool mPressedMouseButtons[static_cast<size_t>(MouseButton::END)] = {};
	int mMouseWheelDelta = 0;

	NetworkManager* network{ nullptr };

	int clientID;
};