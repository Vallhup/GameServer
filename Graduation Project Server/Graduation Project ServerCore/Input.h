#pragma once

enum class InputButton : uint32_t
{
	None = 0,
	Attack = 1u << 0,
	Parry  = 1u << 1,
	LockOn = 1u << 2
};

constexpr uint32_t Button(InputButton button)
{
	return static_cast<uint32_t>(button);
}

template<InputButton... Btns>
constexpr uint32_t Mask()
{
	return (0u | ... | Button(Btns));
}

constexpr uint32_t EVENT_MASK = Mask<InputButton::Attack, InputButton::Parry>();
constexpr uint32_t TOGGLE_MASK = Mask<InputButton::LockOn>();

namespace InputStruct {
	using namespace std::chrono;

	struct InputCommand {
		uint32_t clientSeq;
		uint32_t buttons;
		uint32_t clientTimeMs;
	};

	struct InputState {
		uint32_t buttonDown;
		uint32_t buttonHold;
		uint32_t buttonUp;
		uint32_t buttonPrev;
	};

	struct IntentEvent {
		enum class IntentType{ Attack, Parry, LockOn } type;
		uint32_t clientSeq;
		high_resolution_clock::time_point serverEnqueueTime;
		high_resolution_clock::time_point expireTime;
	};
}