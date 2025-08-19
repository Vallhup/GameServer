#include "pch.h"
#include "Camera.h"
#include "DX12Core.h"
#include "Input.h"

void Camera::Initialize()
{
	position = { 0.0f, 0.0f, 0.0f };
    targetPosition = { 0.0f, 0.0f, 1.0f };

	yaw = 0.0f;
	pitch = -26.57f;
	moveSpeed = 5.0f;
	rotateSpeed = 90.0f;

    lastMousePos = GET(Input).GetMousePosition();

    UpdateForwardAndRight();

    //OutputDebugStringA("Camera init!!\n");
}

void Camera::InitCameraPositionFromCharacter(const XMFLOAT3& pos)
{
    targetPosition = { pos.x, pos.y + 2.0f, pos.z };
    position = { targetPosition.x, targetPosition.y + 2.0f, targetPosition.z + 4.0f };
}

void Camera::Update(DX12Core& core, float deltaTime)
{
    UpdateInputtoCamLogic(deltaTime);
    UpdateCameraMatrices(core);
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    ChangeAngleByInput(deltaTime);
}

void Camera::UpdateCameraMatrices(DX12Core& core)
{
    XMVECTOR eyePos = XMLoadFloat3(&position);
    XMVECTOR lookAt = XMLoadFloat3(&targetPosition);
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX matView = XMMatrixLookAtLH(eyePos, lookAt, upDir);

    float aspectRatio = static_cast<float>(WinSize.x) / static_cast<float>(WinSize.y);
    XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);

    matView = XMMatrixTranspose(matView);
    matProj = XMMatrixTranspose(matProj);

    core.GetFrameCB()->CopyData(&matView, sizeof(XMMATRIX), 0);
    core.GetFrameCB()->CopyData(&matProj, sizeof(XMMATRIX), sizeof(XMMATRIX));
}

void Camera::UpdateForwardAndRight()
{
    camForward = {
        cos(XMConvertToRadians(pitch)) * sin(XMConvertToRadians(yaw)),
        sin(XMConvertToRadians(pitch)),
        cos(XMConvertToRadians(pitch)) * cos(XMConvertToRadians(yaw))
    };

    XMVECTOR forward = XMLoadFloat3(&camForward);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR rightVec = XMVector3Cross(up, forward);    // up & forward 인자 순서 반대하면 leftVec
    rightVec = XMVector3Normalize(rightVec);
    XMStoreFloat3(&right, rightVec);
}

void Camera::ChangeAngleByInput(float deltaTime)
{
    if (GET(Input).GetKey(VK_LEFT))  yaw -= rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_RIGHT)) yaw += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_UP))    pitch += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_DOWN))  pitch -= rotateSpeed * deltaTime;

    XMFLOAT2 currentMousePos = GET(Input).GetMousePosition();

    float deltaX = currentMousePos.x - lastMousePos.x;
    float deltaY = currentMousePos.y - lastMousePos.y;

    yaw += deltaX * mouseSensitivity;
    pitch += deltaY * mouseSensitivity;  

    lastMousePos = currentMousePos;

    constexpr float MAX_PITCH_DEGREE = 89.0f;   // 90도 찍히면 짐벌락걸려요~
    pitch = max(-MAX_PITCH_DEGREE, min(MAX_PITCH_DEGREE, pitch));
}

void Camera::SetCameraPosition(const XMFLOAT3& pos)
{
    targetPosition = { pos.x, pos.y + 2.0f, pos.z };

    float distance = sqrt(4 * 4 + 2 * 2); 

    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);

    position.x = targetPosition.x + distance * cos(radPitch) * sin(radYaw);
    position.y = targetPosition.y + distance * sin(radPitch);
    position.z = targetPosition.z + distance * cos(radPitch) * cos(radYaw);
}