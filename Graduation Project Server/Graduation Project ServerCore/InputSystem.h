#pragma once

enum class TestInput : uint8_t {
	Move,
	Attack,
	Max
};

enum class TestInputType : uint8_t {
	KeyDown,
	KeyUp
};

struct TestInputPacket {
	int id;
	char key;
	char inputType;
};

class IInputable;

class InputSystem {
public:
	static constexpr size_t INPUT_KEY_COUNT = static_cast<size_t>(TestInput::Max);
	using KeyState = std::bitset<INPUT_KEY_COUNT>;

public:
	void Register(int id, IInputable* i);
	void Deregister(int id);

	void HandleInput(const TestInputPacket& packet);

private:
	std::unordered_map<int, IInputable*> _inputables;
};

