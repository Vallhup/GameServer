#pragma once

class SoundManager
{
public:
	void Init();
	void Release();

	void PlayBGM();
	void ChangeBGM(const char* path, bool in = false);

	void SetVolume(float volume);
	float GetVolume();

private:
	FMOD::System* system = nullptr;
	FMOD::Sound* bgmSound = nullptr;
	FMOD::Channel* bgmChannel = nullptr;

	float currentVolume = 0.2f;
};
