#pragma once

using namespace FMOD;

class SoundManager
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Release();

    void PlayBGM(const char* path);
    void StopBGM(float fadeSeconds = 0.0f);
    void SetBGMVolume(float volume);

    void PreloadSFX(const char* path);
    void PlaySFX(const char* path);
    void SetSFXVolume(float volume);

private:
    System* system = nullptr;
    ChannelGroup* bgmGroup = nullptr;  
    ChannelGroup* sfxGroup = nullptr;
    Channel* bgmChannel = nullptr;

    Channel* fadeChannel = nullptr;
    float fadeTimer = 0.0f;
    float fadeDuration = 0.0f;

    unordered_map<string, Sound*> bgmCache;
    unordered_map<string, Sound*> sfxCache;
    string currentBGMPath;
};
