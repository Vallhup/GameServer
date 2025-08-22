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

constexpr size_t INPUT_KEY_COUNT = static_cast<size_t>(TestInput::Max);

class InputSystem {
public:
	using KeyState = std::bitset<INPUT_KEY_COUNT>;
	
public:



	void HandleInput(const TestInputPacket& packet);

public:
	const KeyState& GetState(int sessionId) const;

private:
	std::vector<IInputable> _inputs;
};

