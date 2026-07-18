#include "pch.h"
#include "VideoPlayer.h"
#include <mfapi.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class VideoNotify : public IMFMediaEngineNotify
{
public:
    explicit VideoNotify(VideoPlayer* owner) : mOwner(owner) {}
    STDMETHODIMP EventNotify(DWORD ev, DWORD_PTR, DWORD) override
    {
        if (ev == MF_MEDIA_ENGINE_EVENT_CANPLAY) mOwner->OnAudioReady();
        return S_OK;
    }
    STDMETHODIMP QueryInterface(REFIID id, void** o) override
    {
        if (id == __uuidof(IMFMediaEngineNotify) || id == IID_IUnknown)
        { *o = static_cast<IMFMediaEngineNotify*>(this); AddRef(); return S_OK; }
        *o = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&mRef); }
    STDMETHODIMP_(ULONG) Release() override { ULONG r = InterlockedDecrement(&mRef); if (!r) delete this; return r; }

private:
    VideoPlayer* mOwner; 
    LONG mRef = 1;
};

bool VideoPlayer::Open(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, const std::wstring& path)
{
    mDevice = device;
    if (FAILED(MFStartup(MF_VERSION))) return false;
    mMfStarted = true;

    std::wstring abs = path;
    {
        wchar_t full[MAX_PATH] = {};
        if (GetFullPathNameW(path.c_str(), MAX_PATH, full, nullptr) > 0)
            abs = full;
    }

    ComPtr<IMFAttributes> rattr;
    MFCreateAttributes(&rattr, 1);
    rattr->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    if (FAILED(MFCreateSourceReaderFromURL(abs.c_str(), rattr.Get(), &mReader)))
        return false;

    ComPtr<IMFMediaType> outType;
    MFCreateMediaType(&outType);
    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE,    MFVideoFormat_RGB32);
    if (FAILED(mReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outType.Get())))
        return false;
    mReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

    ComPtr<IMFMediaType> cur;
    mReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &cur);
    MFGetAttributeSize(cur.Get(), MF_MT_FRAME_SIZE, &mW, &mH);
    UINT32 strideU = 0;
    if (SUCCEEDED(cur->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideU)))
        mStride = (LONG)strideU;
    else
        mStride = (LONG)(mW * 4);   

    if (!CreateTexture(srvCpu)) return false;

    mNotify.Attach(new VideoNotify(this));
    ComPtr<IMFAttributes> mattr;
    MFCreateAttributes(&mattr, 1);
    mattr->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, mNotify.Get());

    ComPtr<IMFMediaEngineClassFactory> factory;
    if (SUCCEEDED(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
    {
        if (SUCCEEDED(factory->CreateInstance(MF_MEDIA_ENGINE_AUDIOONLY, mattr.Get(), &mEngine)))
        {
            mEngine->SetAutoPlay(TRUE);   
            mEngine->SetMuted(FALSE);
            mEngine->SetVolume(0.1);
            BSTR url = SysAllocString(abs.c_str());
            mEngine->SetSource(url);
            SysFreeString(url);
        }
    }
    return true;
}

bool VideoPlayer::CreateTexture(D3D12_CPU_DESCRIPTOR_HANDLE srvCpu)
{
    D3D12_RESOURCE_DESC td = {};
    td.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width            = mW;
    td.Height           = mH;
    td.DepthOrArraySize = 1;
    td.MipLevels        = 1;
    td.Format           = DXGI_FORMAT_B8G8R8A8_TYPELESS;   
    td.SampleDesc.Count = 1;

    CD3DX12_HEAP_PROPERTIES defHeap(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(mDevice->CreateCommittedResource(&defHeap, D3D12_HEAP_FLAG_NONE, &td,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&mTex))))
        return false;

    UINT rows; UINT64 rowBytes, total;
    mDevice->GetCopyableFootprints(&td, 0, 1, 0, &mFootprint, &rows, &rowBytes, &total);
    mFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;   

    CD3DX12_HEAP_PROPERTIES upHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bd = CD3DX12_RESOURCE_DESC::Buffer(total);
    if (FAILED(mDevice->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUpload))))
        return false;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels     = 1;
    mDevice->CreateShaderResourceView(mTex.Get(), &srv, srvCpu);
    return true;
}

void VideoPlayer::OnAudioReady()
{
    mReady = true;   
}

void VideoPlayer::EnsurePending()
{
    if (mHavePending || mVideoEos) return;
    DWORD flags = 0; LONGLONG pts = 0;
    ComPtr<IMFSample> sample;
    if (FAILED(mReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
            nullptr, &flags, &pts, &sample)))
        return;
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { mVideoEos = true; return; }
    if (!sample) return;
    mPending = sample;
    mPendingPts = pts;
    mHavePending = true;
}

void VideoPlayer::UploadSample(ID3D12GraphicsCommandList* cmdList, IMFSample* sample)
{
    ComPtr<IMFMediaBuffer> buf;
    if (FAILED(sample->ConvertToContiguousBuffer(&buf))) return;
    BYTE* src = nullptr; DWORD len = 0;
    if (FAILED(buf->Lock(&src, nullptr, &len))) return;

    const UINT dstPitch = mFootprint.Footprint.RowPitch;
    const UINT srcAbs   = (mStride < 0) ? (UINT)(-mStride) : (UINT)mStride;

    BYTE* dst = nullptr;
    if (SUCCEEDED(mUpload->Map(0, nullptr, reinterpret_cast<void**>(&dst))))
    {
        for (UINT y = 0; y < mH; ++y)
        {
            const BYTE* srcRow = (mStride < 0)
                ? src + (size_t)(mH - 1 - y) * srcAbs   
                : src + (size_t)y * srcAbs;             
            const uint32_t* s = reinterpret_cast<const uint32_t*>(srcRow);
            uint32_t* d = reinterpret_cast<uint32_t*>(dst + (size_t)y * dstPitch);
            for (UINT x = 0; x < mW; ++x)
                d[x] = s[x] | 0xFF000000u;             
        }
        mUpload->Unmap(0, nullptr);
    }
    buf->Unlock();
    (void)len;

    if (mState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        auto b = CD3DX12_RESOURCE_BARRIER::Transition(mTex.Get(), mState, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(1, &b);
    }
    CD3DX12_TEXTURE_COPY_LOCATION d(mTex.Get(), 0);
    CD3DX12_TEXTURE_COPY_LOCATION s(mUpload.Get(), mFootprint);
    cmdList->CopyTextureRegion(&d, 0, 0, 0, &s, nullptr);
    auto b2 = CD3DX12_RESOURCE_BARRIER::Transition(mTex.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &b2);
    mState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void VideoPlayer::Update(ID3D12GraphicsCommandList* cmdList)
{
    if (!mStarted)
    {
        mStart = std::chrono::steady_clock::now();
        mStarted = true;
        mReady = true;
    }

    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - mStart).count();
    LONGLONG t100 = (LONGLONG)(elapsed * 1e7);

    int cap = 0;
    EnsurePending();
    while (mHavePending && mPendingPts <= t100 && cap < 4)
    {
        UploadSample(cmdList, mPending.Get());
        mPending.Reset();
        mHavePending = false;
        EnsurePending();
        ++cap;
    }
}

void VideoPlayer::Close()
{
    if (mEngine) { mEngine->Shutdown(); mEngine.Reset(); }
    mNotify.Reset();
    mReader.Reset();
    mPending.Reset();
    mUpload.Reset();
    mTex.Reset();
    mDevice.Reset();
    mReady = false;
    if (mMfStarted) { MFShutdown(); mMfStarted = false; }
}
