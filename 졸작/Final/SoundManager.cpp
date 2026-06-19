#include "pch.h"
#include "SoundManager.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "MainCharacter.h"
#include "Transform.h"

void SoundManager::Initialize()
{
    FMOD::System_Create(&system);
    system->init(512, FMOD_INIT_NORMAL, nullptr);

    system->createChannelGroup("BGM", &bgmGroup);
    system->createChannelGroup("SFX", &sfxGroup);

    bgmGroup->setVolume(BGM_BASE_VOLUME);   
    sfxGroup->setVolume(SFX_BASE_VOLUME);
}

void SoundManager::Update(float deltaTime)
{
    if (fadeChannel)
    {
        fadeTimer -= deltaTime;
        if (fadeTimer <= 0.0f)
        {
            fadeChannel->stop();
            fadeChannel = nullptr;

            if (hasPendingBGM)
            {
                hasPendingBGM = false;
                StartBGM(pendingBGMPath.c_str(), pendingFadeIn, pendingLoop);
                pendingBGMPath.clear();
            }
        }
        else
        {
            fadeChannel->setVolume(fadeTimer / fadeDuration);
        }
    }

    if (bgmChannel && fadeInTimer > 0.0f)
    {
        fadeInTimer -= deltaTime;
        if (fadeInTimer <= 0.0f)
        {
            fadeInTimer = 0.0f;
            bgmChannel->setVolume(1.0f);
        }
        else
        {
            bgmChannel->setVolume(1.0f - fadeInTimer / fadeInDuration);
        }
    }

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

    for (auto& pair : positionalSfxCache)
    {
        if (pair.second)
            pair.second->release();
    }
    positionalSfxCache.clear();

    if (system)
    {
        system->close();
        system->release();
        system = nullptr;
    }
}

void SoundManager::PlayBGM(const char* path, float fadeInSeconds, bool loop)
{
    if (currentBGMPath == path)
        return;

    if (fadeChannel)
    {
        pendingBGMPath = path;
        pendingFadeIn = fadeInSeconds;
        pendingLoop = loop;
        hasPendingBGM = true;
        return;
    }

    StartBGM(path, fadeInSeconds, loop);
}

void SoundManager::StartBGM(const char* path, float fadeInSeconds, bool loop)
{
    if (bgmChannel)
    {
        if (fadeInSeconds > 0.0f)
        {
            if (fadeChannel)
                fadeChannel->stop();
            fadeChannel = bgmChannel;
            fadeDuration = fadeInSeconds;
            fadeTimer = fadeInSeconds;
        }
        else
        {
            bgmChannel->stop();
        }
    }

    string key(path);

    if (bgmCache.find(key) == bgmCache.end())
        system->createSound(path, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &bgmCache[key]);

    currentBGMPath = path;
    system->playSound(bgmCache[key], bgmGroup, false, &bgmChannel);
    bgmChannel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

    if (fadeInSeconds > 0.0f)
    {
        fadeInDuration = fadeInSeconds;
        fadeInTimer = fadeInSeconds;
        bgmChannel->setVolume(0.0f);   
    }
    else
    {
        fadeInTimer = 0.0f;
        bgmChannel->setVolume(1.0f);
    }
}

void SoundManager::StopBGM(float fadeSeconds)
{
    if (bgmChannel)
    {
        if (fadeSeconds > 0.0f)
        {
            if (fadeChannel)
                fadeChannel->stop();

            fadeChannel = bgmChannel;
            fadeDuration = fadeSeconds;
            fadeTimer = fadeSeconds;
        }
        else
        {
            bgmChannel->stop();
        }

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

void SoundManager::PreloadEverySFX()
{
    PreloadSFX("../Assets/Music/SFX/Parry.mp3");
    PreloadSFX("../Assets/Music/SFX/ButtonPress.mp3");
    PreloadSFX("../Assets/Music/SFX/CutMonster.mp3");
    PreloadSFX("../Assets/Music/SFX/CutFinalBoss.mp3");
    PreloadSFX("../Assets/Music/SFX/CharacterCut.mp3");
    PreloadSFX("../Assets/Music/SFX/Foot.mp3");
    PreloadSFX("../Assets/Music/SFX/SwingSword.mp3");
    PreloadSFX("../Assets/Music/SFX/Roll.mp3");
    PreloadSFX("../Assets/Music/SFX/Guard.mp3");
    PreloadSFX("../Assets/Music/SFX/CinematicExplosion.mp3");
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

void SoundManager::PlaySFX3D(const char* path, const XMFLOAT3& worldPos)
{
    string key(path);
    Sound* sound = nullptr;

    auto it = positionalSfxCache.find(key);
    if (it != positionalSfxCache.end())
    {
        sound = it->second;
    }
    else
    {
        system->createSound(path, FMOD_2D, nullptr, &sound);
        positionalSfxCache[key] = sound;
    }

    float volume = 1.0f;
    XMFLOAT3 listenerPos;
    if (GetListenerPosition(listenerPos))
    {
        XMVECTOR diff = XMVectorSubtract(XMLoadFloat3(&worldPos), XMLoadFloat3(&listenerPos));
        const float dist = XMVectorGetX(XMVector3Length(diff));
        volume = clamp((SFX3D_MAX_DISTANCE - dist) / (SFX3D_MAX_DISTANCE - SFX3D_MIN_DISTANCE), 0.0f, 1.0f);
    }

    Channel* channel = nullptr;
    system->playSound(sound, sfxGroup, true, &channel);
    if (channel)
    {
        channel->setVolume(volume);
        channel->setPaused(false);
    }
}

bool SoundManager::GetListenerPosition(XMFLOAT3& outPos) const
{
    Scene* scene = SCENE_MANAGER->GetCurrentScene();
    if (!scene)
        return false;

    MainCharacter* player = scene->GetMyPlayer();
    if (!player)
        return false;

    if (auto* tf = player->GetComponent<Transform>())
    {
        outPos = tf->GetPosition();
        return true;
    }
    return false;
}

void SoundManager::StopAllSFX()
{
    if (sfxGroup)
        sfxGroup->stop();
}

void SoundManager::SetSFXVolume(float volume)
{
    if (sfxGroup)
        sfxGroup->setVolume(clamp(volume, 0.0f, 1.0f));
}