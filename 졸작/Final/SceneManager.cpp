#include "pch.h"
#include "SceneManager.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"
#include "Importer.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Input.h"
#include "Timer.h"
#include "DescriptorHeap.h"
#include "Texture.h"

SceneManager& SceneManager::Get()
{
	static SceneManager sceneManager;
	return sceneManager;
}

SceneManager::~SceneManager()
{
	Release();
}

void SceneManager::Initialize(HWND hwnd)
{
	mHwnd = hwnd;


}

void SceneManager::Update(const float deltaTime)
{
    if (mCurrentScene)
    {
        mCurrentScene->Update(deltaTime);
    }
}

void SceneManager::Render()
{
    if (mCurrentScene)
    {
        mCurrentScene->Render();
    }

    // === 간단한 카메라 컨트롤 테스트 ===
    static XMFLOAT3 cameraPos = { 0.0f, 2.0f, -5.0f };
    static float yaw = 0.0f;
    static float pitch = 0.0f;

    float deltaTime = GET(Timer).GetDeltaTime();
    float moveSpeed = 5.0f;
    float rotateSpeed = 90.0f;

    // 방향키로 카메라 회전
    if (GET(Input).GetKey(VK_LEFT))  yaw -= rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_RIGHT)) yaw += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_UP))    pitch += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_DOWN))  pitch -= rotateSpeed * deltaTime;

    // 피치 제한
    pitch = max(-89.0f, min(89.0f, pitch));

    // 카메라 전방 벡터 계산
    float yawRad = XMConvertToRadians(yaw);
    float pitchRad = XMConvertToRadians(pitch);
    XMFLOAT3 forward = {
        cos(pitchRad) * sin(yawRad),
        sin(pitchRad),
        cos(pitchRad) * cos(yawRad)
    };
    XMFLOAT3 right = { forward.z, 0.0f, -forward.x };

    // WASD로 이동
    XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (GET(Input).GetKey('W'))  // 카메라가 보는 방향으로 앞으로 (Y축 포함)
    {
        moveDir.x += forward.x;
        moveDir.y += forward.y;  // 이거 추가!
        moveDir.z += forward.z;
    }
    if (GET(Input).GetKey('S'))  // 카메라가 보는 방향 반대로 뒤로
    {
        moveDir.x -= forward.x;
        moveDir.y -= forward.y;  // 이거 추가!
        moveDir.z -= forward.z;
    }
    if (GET(Input).GetKey('A'))  // 카메라 기준 왼쪽으로
    {
        moveDir.x -= right.x;
        moveDir.z -= right.z;
    }
    if (GET(Input).GetKey('D'))  // 카메라 기준 오른쪽으로
    {
        moveDir.x += right.x;
        moveDir.z += right.z;
    }

    // 이동 적용
    float moveDistance = moveSpeed * deltaTime;
    cameraPos.x += moveDir.x * moveDistance;
    cameraPos.y += moveDir.y * moveDistance;
    cameraPos.z += moveDir.z * moveDistance;

    // 뷰 행렬 생성
    XMVECTOR eyePos = XMLoadFloat3(&cameraPos);
    XMVECTOR lookAt = XMVectorAdd(eyePos, XMLoadFloat3(&forward));
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX matView = XMMatrixLookAtLH(eyePos, lookAt, upDir);

    XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 1000.0f);

    matView = XMMatrixTranspose(matView);
    matProj = XMMatrixTranspose(matProj);

    GET(DX12Graphics).GetFrameCB()->CopyData(&matView, sizeof(XMMATRIX), 0);
    GET(DX12Graphics).GetFrameCB()->CopyData(&matProj, sizeof(XMMATRIX), sizeof(XMMATRIX));

    // === FBX 캐릭터 렌더링 추가 ===
    static Importer importer;
    static unique_ptr<VertexIndexBuffer> fbxMesh;

    if (!fbxMesh) {  // 한 번만 로드
        if (importer.LoadModel(L"../FBXOutput/knight_Tpose")) {
            const MeshData& mesh = importer.GetMesh();

            fbxMesh = make_unique<VertexIndexBuffer>();
            fbxMesh->Initialize(
                GET(DX12Graphics).GetDevice()->GetDevice().Get(),
                GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
                mesh.vertices,
                mesh.indices
            );

            OutputDebugStringA("FBX Mesh created for rendering!\n");
        }
    }

    // 캐릭터 렌더링
    if (fbxMesh) {
        // 월드 행렬 (위치, 크기 조정)
        XMMATRIX worldMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f) *  // 크기 조정
            XMMatrixTranslation(0.0f, 0.0f, -1.0f);   // 위치

        // 상수 버퍼 업데이트 (기존 구조 활용)
        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(worldMatrix);
        objConstants.useTexture = 0;
        objConstants.heightScale = 1.0f;
        GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

        // 파이프라인 설정
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetPipelineState(GET(DX12Graphics).GetShader()->GetTransparentPSO());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

        // === 디스크립터 힙 설정 (중요!) ===
        ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


        // 상수 버퍼 바인딩
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

        // 메쉬 렌더링
        fbxMesh->Bind(GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get());
        fbxMesh->Draw(GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get());
    }
}

void SceneManager::Release()
{
    for (auto& scene : mScenes)
    {
        if (scene)
        {
            scene->Release();  
            scene.reset();     
        }
    }

    mCurrentScene = nullptr;
}

Scene* SceneManager::GetCurrentScene() const
{
	return mCurrentScene;
}

void SceneManager::ChangeScene(SceneType type)
{
    size_t index = static_cast<size_t>(type);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    mCurrentScene = mScenes[index].get();
    
    GET(DX12Graphics).GetCmdQueue()->SetBackgroundColor(mCurrentScene->GetBackgroundColor());

    if (mCurrentScene)
    {
        mCurrentScene->Reset();
    }
}