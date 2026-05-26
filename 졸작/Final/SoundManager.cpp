#include "pch.h"
#include "SoundManager.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"

void SoundManager::Initialize()
{
    FMOD::System_Create(&system);
    system->init(512, FMOD_INIT_NORMAL, nullptr);

    system->createChannelGroup("BGM", &bgmGroup);
    system->createChannelGroup("SFX", &sfxGroup);

    bgmGroup->setVolume(0.3f);
    sfxGroup->setVolume(0.5f);
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

            if (hasPendingBGM)   // 페이드아웃 완료 → 예약된 곡 페이드인 시작
            {
                hasPendingBGM = false;
                StartBGM(pendingBGMPath.c_str(), pendingFadeIn);
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

    UpdateListener();

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

    for (auto& pair : sfx3DCache)
    {
        if (pair.second)
            pair.second->release();
    }
    sfx3DCache.clear();

    if (system)
    {
        system->close();
        system->release();
        system = nullptr;
    }
}

void SoundManager::PlayBGM(const char* path, float fadeInSeconds)
{
    if (currentBGMPath == path)
        return;

    // 이전 곡이 페이드아웃 중이면 끝난 뒤 시작하도록 예약(순차 전환)
    if (fadeChannel)
    {
        pendingBGMPath = path;
        pendingFadeIn = fadeInSeconds;
        hasPendingBGM = true;
        return;
    }

    StartBGM(path, fadeInSeconds);
}

void SoundManager::StartBGM(const char* path, float fadeInSeconds)
{
    if (bgmChannel)
        bgmChannel->stop();

    string key(path);

    if (bgmCache.find(key) == bgmCache.end())
        system->createSound(path, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &bgmCache[key]);

    currentBGMPath = path;
    system->playSound(bgmCache[key], bgmGroup, false, &bgmChannel);

    if (fadeInSeconds > 0.0f)
    {
        fadeInDuration = fadeInSeconds;
        fadeInTimer = fadeInSeconds;
        bgmChannel->setVolume(0.0f);   // 0에서 시작해 Update에서 1까지 램프
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
            // 이전에 페이드 중이던 채널이 남아 있으면 즉시 정리
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

    auto it = sfx3DCache.find(key);
    if (it != sfx3DCache.end())
    {
        sound = it->second;
    }
    else
    {
        system->createSound(path, FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, &sound);
        if (sound)
            sound->set3DMinMaxDistance(SFX3D_MIN_DISTANCE, SFX3D_MAX_DISTANCE);
        sfx3DCache[key] = sound;
    }

    // 위치 없이 한 프레임 새는 걸 막기 위해 정지 상태로 재생 → 위치 지정 → 해제
    Channel* channel = nullptr;
    system->playSound(sound, sfxGroup, true, &channel);
    if (channel)
    {
        const FMOD_VECTOR pos{ worldPos.x, worldPos.y, worldPos.z };
        const FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };
        channel->set3DAttributes(&pos, &vel);
        channel->setPaused(false);
    }
}

void SoundManager::UpdateListener()
{
    if (!system)
        return;

    Scene* scene = SCENE_MANAGER->GetCurrentScene();
    if (!scene)
        return;

    Camera* camera = scene->GetCamera();
    if (!camera)
        return;

    const XMFLOAT3 pos = camera->GetPosition();
    const XMFLOAT3 fwd = camera->GetForward();

    const FMOD_VECTOR fpos{ pos.x, pos.y, pos.z };
    const FMOD_VECTOR fvel{ 0.0f, 0.0f, 0.0f };
    const FMOD_VECTOR ffwd{ fwd.x, fwd.y, fwd.z };
    const FMOD_VECTOR fup{ 0.0f, 1.0f, 0.0f };
    system->set3DListenerAttributes(0, &fpos, &fvel, &ffwd, &fup);
}

void SoundManager::SetSFXVolume(float volume)
{
    if (sfxGroup)
        sfxGroup->setVolume(clamp(volume, 0.0f, 1.0f));
}