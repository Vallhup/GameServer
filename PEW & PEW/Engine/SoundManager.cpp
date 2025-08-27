#include "pch.h"
#include "SoundManager.h"

void SoundManager::Init()
{
    char currentPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentPath);
    std::cout << "[SOUND] Current directory: " << currentPath << std::endl;

    FMOD::System_Create(&system);
    system->init(512, FMOD_INIT_NORMAL, 0);

    system->createSound("music/Startsong.mp3", FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE, 0, &bgmSound);
    system->playSound(bgmSound, 0, true, &bgmChannel);

    if (bgmChannel)
        bgmChannel->setVolume(currentVolume);
}

void SoundManager::Release()
{
    if (bgmSound)
    {
        bgmSound->release();
        bgmSound = nullptr;
    }

    if (system)
    {
        system->close();
        system->release();
        system = nullptr;
    }
}

void SoundManager::PlayBGM()
{
    if (bgmSound)
        bgmChannel->setPaused(false);
}

void SoundManager::ChangeBGM(const char* path, bool in)
{
    if (bgmChannel)
    {
        bgmChannel->stop();
        bgmChannel = nullptr;
    }

    if (bgmSound)
    {
        bgmSound->release();
        bgmSound = nullptr;
    }

    system->createSound(path, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, 0, &bgmSound);
    system->playSound(bgmSound, 0, in, &bgmChannel);

    if (bgmChannel)
        bgmChannel->setVolume(currentVolume);
}

void SoundManager::SetVolume(float volume)
{
    currentVolume = glm::clamp(volume, 0.0f, 0.5f);
    if (bgmChannel)
        bgmChannel->setVolume(currentVolume);
}

float SoundManager::GetVolume()
{
    return currentVolume;
}
