#pragma once

using namespace FMOD;

class SoundManager
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Release();

    void PlayBGM(const char* path, float fadeInSeconds = 0.0f, bool loop = true);
    void StopBGM(float fadeSeconds = 0.0f);

private:
    void StartBGM(const char* path, float fadeInSeconds, bool loop);

public:
    void SetBGMVolume(float volume);

    void PreloadSFX(const char* path);
    void PreloadEverySFX();
    void PlaySFX(const char* path);
    void PlaySFX3D(const char* path, const XMFLOAT3& worldPos);
    void StopAllSFX();
    void SetSFXVolume(float volume);

private:
    bool GetListenerPosition(XMFLOAT3& outPos) const;

    static constexpr float SFX3D_MIN_DISTANCE = 3.0f;
    static constexpr float SFX3D_MAX_DISTANCE = 20.0f;

    static constexpr float BGM_BASE_VOLUME = 0.3f;
    static constexpr float SFX_BASE_VOLUME = 0.5f;

    System* system = nullptr;
    ChannelGroup* bgmGroup = nullptr;  
    ChannelGroup* sfxGroup = nullptr;
    Channel* bgmChannel = nullptr;

    Channel* fadeChannel = nullptr;
    float fadeTimer = 0.0f;
    float fadeDuration = 0.0f;

    float fadeInTimer = 0.0f;
    float fadeInDuration = 0.0f;

    string pendingBGMPath;
    float  pendingFadeIn = 0.0f;
    bool   pendingLoop = true;
    bool   hasPendingBGM = false;

    unordered_map<string, Sound*> bgmCache;
    unordered_map<string, Sound*> sfxCache;
    unordered_map<string, Sound*> positionalSfxCache;
    string currentBGMPath;
};
