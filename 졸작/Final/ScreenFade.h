#pragma once

enum class State { Idle, FadingOut, Black, FadingIn };

class ScreenFade
{
public:
	void FadeOut(float duration = 1.0f);
	void FadeIn(float duration = 1.0f);

	void Update(float deltaTime);

	float GetAlpha() const { return alpha; }
	State GetState() const { return state; }
	bool  IsActive() const { return state != State::Idle; }   
	bool  IsBlack() const { return state == State::Black; }   

	void SetOnFadedOut(std::function<void()> cb) { onFadedOut = std::move(cb); }
	void SetOnFadedIn(std::function<void()> cb) { onFadedIn = std::move(cb); }

private:
	State state = State::Idle;
	float alpha = 0.0f;
	float duration = 1.0f;
	float elapsed = 0.0f;
	std::function<void()> onFadedOut;
	std::function<void()> onFadedIn;
};
