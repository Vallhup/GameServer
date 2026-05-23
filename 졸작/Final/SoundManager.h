#pragma once

using namespace FMOD;

class SoundManager
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Release();

    void PlayBGM(const char* path, float fadeInSeconds = 0.0f);
    void StopBGM(float fadeSeconds = 0.0f);

private:
    void StartBGM(const char* path, float fadeInSeconds);

public:
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

    float fadeInTimer = 0.0f;
    float fadeInDuration = 0.0f;

    // 페이드아웃이 끝난 뒤 시작하도록 예약된 BGM(순차 전환)
    string pendingBGMPath;
    float  pendingFadeIn = 0.0f;
    bool   hasPendingBGM = false;

    unordered_map<string, Sound*> bgmCache;
    unordered_map<string, Sound*> sfxCache;
    string currentBGMPath;
};
