#include "pch.h"
#include "SoundManager.h"

void SoundManager::Initialize()
{
    FMOD::System_Create(&system);
    system->init(512, FMOD_INIT_NORMAL, nullptr);

    system->createChannelGroup("BGM", &bgmGroup);
    system->createChannelGroup("SFX", &sfxGroup);

    bgmGroup->setVolume(0.3f);
    sfxGroup->setVolume(0.5f);
}

void SoundManager::Update()
{
    if (system)
        system->update();
}

void SoundManager::Release()
{
    for (auto& pair : bgmCache)
        pair.second->release();
    bgmCache.clear();

    for (auto& pair : sfxCache)
    {
        if (pair.second)
            pair.second->release();
    }
    sfxCache.clear();

    if (system)
    {
        system->close();
        system->release();
        system = nullptr;
    }
}

void SoundManager::PlayBGM(const char* path)
{
    if (currentBGMPath == path)
        return;

    if (bgmChannel)
        bgmChannel->stop();

    string key(path);

    if (bgmCache.find(key) == bgmCache.end())
        system->createSound(path, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &bgmCache[key]);
    
    currentBGMPath = path;
    system->playSound(bgmCache[key], bgmGroup, false, &bgmChannel);
}

void SoundManager::StopBGM()
{
    if (bgmChannel)
    {
        bgmChannel->stop();
        bgmChannel = nullptr;
    }

    currentBGMPath.clear();
}

void SoundManager::SetBGMVolume(float volume)
{
    if (bgmGroup)
        bgmGroup->setVolume(clamp(volume, 0.0f, 1.0f));
}

void SoundManager::PreloadSFX(const char* path)
{
    string key(path);
    if (sfxCache.find(key) != sfxCache.end())
        return;

    Sound* sound = nullptr;
    system->createSound(path, FMOD_DEFAULT, nullptr, &sound);
    sfxCache[key] = sound;
}

void SoundManager::PlaySFX(const char* path)
{
    string key(path);
    Sound* sound = nullptr;

    auto it = sfxCache.find(key);
    if (it != sfxCache.end())
    {
        sound = it->second;
    }
    else
    {
        system->createSound(path, FMOD_DEFAULT, nullptr, &sound);
        sfxCache[key] = sound;
    }

    system->playSound(sound, sfxGroup, false, nullptr);
}

void SoundManager::SetSFXVolume(float volume)
{
    if (sfxGroup)
        sfxGroup->setVolume(clamp(volume, 0.0f, 1.0f));
}