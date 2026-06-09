#pragma once
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfmediaengine.h>
#include <chrono>

class VideoPlayer
{
public:
    bool Open(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, const std::wstring& path);
    void Update(ID3D12GraphicsCommandList* cmdList);
    bool IsReady() const { return mReady; }
    bool IsEnded() const { return mVideoEos; }
    UINT Width()  const { return mW; }
    UINT Height() const { return mH; }
    void Close();

    void OnAudioReady();                 
    void OnEnded() { mEnded = true; }    

private:
    bool CreateTexture(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu);
    void EnsurePending();                
    void UploadSample(ID3D12GraphicsCommandList* cmdList, IMFSample* sample);

    ComPtr<ID3D12Device>            mDevice;
    ComPtr<IMFSourceReader>         mReader;     
    ComPtr<IMFMediaEngine>          mEngine;     
    ComPtr<IMFMediaEngineNotify>    mNotify;
    ComPtr<ID3D12Resource>          mTex;
    ComPtr<ID3D12Resource>          mUpload;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mFootprint{};
    UINT  mW = 0, mH = 0;
    LONG  mStride = 0;                 

    ComPtr<IMFSample> mPending;
    LONGLONG mPendingPts = 0;
    bool mHavePending = false;

    std::chrono::steady_clock::time_point mStart;
    bool mStarted = false;
    bool mReady = false;              
    bool mEnded = false;
    bool mVideoEos = false;
    bool mMfStarted = false;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};
