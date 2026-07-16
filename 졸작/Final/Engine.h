#pragma once

class DX12Core;
class SceneManager;
class NetworkManager;
class SoundManager;
class EffectManager;
class UIManager;
class IConnectionListener;
class ClientPartyState;
class ClientTitleState;

#include "ClientPacketRouter.h"
#include "ClientInboundPacketQueue.h"
#include "ClientWorldTransitionController.h"

class Engine
{
public:
    static Engine& Get();

    void Initialize(HWND hwnd, string_view ip, uint16 port, IConnectionListener& listener);
    void Update(const float deltaTime);  
    void Render();
    void Shutdown();  
    void ShowFps();

    HWND GetHwnd() const { return mHwnd; }

    SceneManager* GetSceneManager() const { return sceneManager.get(); }
    NetworkManager* GetNetworkManager() const { return networkManager.get(); }
    SoundManager* GetSoundManager() const { return soundManager.get(); }
    EffectManager* GetEffectManager() const { return effectManager.get(); }
    UIManager* GetUIManager() const { return uiManager.get(); }
    ClientPartyState* GetPartyState() const { return partyState.get(); }
    ClientTitleState* GetTitleState() const { return titleState.get(); }
    ClientInboundPacketQueue* GetInboundQueue() const { return inboundQueue.get(); }

    ClientWorldTransitionController& GetWorldTransitionController() 
    {
        return worldTransitionController; 
    }

private:
    void ProcessCheatHotkeys();
    void ProcessWorldTransitionState();
    void EnterPvpReuse();

    HWND mHwnd = nullptr;

    D3D12_VIEWPORT	viewport = {};
    D3D12_RECT		scissorRect = {};

    unique_ptr<DX12Core> graphics;
    unique_ptr<SceneManager> sceneManager;
    unique_ptr<NetworkManager> networkManager;
    unique_ptr<SoundManager> soundManager;
    unique_ptr<EffectManager> effectManager;
    unique_ptr<UIManager> uiManager;

    unique_ptr<ClientInboundPacketQueue> inboundQueue;
    vector<ClientInboundPacket> inboundPackets;

    ClientPacketRouter packetRouter;
    ClientWorldTransitionController worldTransitionController;

    unique_ptr<ClientPartyState> partyState;
    unique_ptr<ClientTitleState> titleState;
};
