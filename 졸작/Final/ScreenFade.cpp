#include "pch.h"
#include "ScreenFade.h"

void ScreenFade::FadeOut(float dur)
{
	duration = (dur > 0.0001f) ? dur : 0.0001f;
	elapsed = alpha * duration;
	state = State::FadingOut;
}

void ScreenFade::FadeIn(float dur)
{
	duration = (dur > 0.0001f) ? dur : 0.0001f;
	elapsed = (1.0f - alpha) * duration;
	state = State::FadingIn;
}

void ScreenFade::Update(float deltaTime)
{
	switch (state)
	{
	case State::Idle:
	case State::Black:
		break;

	case State::FadingOut:
		elapsed += deltaTime;
		alpha = std::clamp(elapsed / duration, 0.0f, 1.0f);
		if (alpha >= 1.0f)
		{
			alpha = 1.0f;
			state = State::Black;
			if (onFadedOut)
			{
				auto cb = onFadedOut;   
				onFadedOut = nullptr;
				cb();
			}
		}
		break;

	case State::FadingIn:
		elapsed += deltaTime;
		alpha = 1.0f - std::clamp(elapsed / duration, 0.0f, 1.0f);
		if (alpha <= 0.0f)
		{
			alpha = 0.0f;
			state = State::Idle;
		}
		break;
	}
}
