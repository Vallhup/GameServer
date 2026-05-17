#pragma once

using namespace FMOD;

class SoundManager
{
public:
    void Initialize();
    void Update();
    void Release();

    void PlayBGM(const char* path);
    void StopBGM();
    void SetBGMVolume(float volume);

    void PreloadSFX(const char* path);
    void PlaySFX(const char* path);
    void SetSFXVolume(float volume);

private:
    System* system = nullptr;
    ChannelGroup* bgmGroup = nullptr;  
    ChannelGroup* sfxGroup = nullptr;
    Channel* bgmChannel = nullptr;

    unordered_map<string, Sound*> bgmCache;
    unordered_map<string, Sound*> sfxCache;
    string currentBGMPath;
};
