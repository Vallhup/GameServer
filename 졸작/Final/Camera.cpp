#include "pch.h"
#include "Camera.h"
#include "DX12Core.h"
#include "Input.h"

void Camera::Initialize()
{
	position = { 0.0f, 2.0f, -5.0f };

	yaw = 0.0f;
	pitch = 0.0f;
	moveSpeed = 5.0f;
	rotateSpeed = 90.0f;

    UpdateForwardAndRight();

    //OutputDebugStringA("Camera init!!\n");
}

void Camera::Update(DX12Core& core, float deltaTime)
{
    UpdateInputtoCamLogic(deltaTime);
    UpdateCameraMatrices(core);
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    UpdateForwardAndRight();
    ChangeAngleByInput(deltaTime);
    ChangePosByInput(deltaTime);
}

void Camera::UpdateCameraMatrices(DX12Core& core)
{
    XMVECTOR eyePos = XMLoadFloat3(&position);
    XMVECTOR lookAt = XMVectorAdd(eyePos, XMLoadFloat3(&camForward));
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX matView = XMMatrixLookAtLH(eyePos, lookAt, upDir);

    XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 1000.0f);

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

void Camera::ChangePosByInput(float deltaTime)
{
    XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (GET(Input).GetKey('W'))
    {
        moveDir.x += camForward.x;
        moveDir.y += camForward.y;
        moveDir.z += camForward.z;
    }
    if (GET(Input).GetKey('S'))
    {
        moveDir.x -= camForward.x;
        moveDir.y -= camForward.y;
        moveDir.z -= camForward.z;
    }
    if (GET(Input).GetKey('A'))
    {
        moveDir.x -= right.x;
        moveDir.z -= right.z;
    }
    if (GET(Input).GetKey('D'))
    {
        moveDir.x += right.x;
        moveDir.z += right.z;
    }

    float moveDistance = moveSpeed * deltaTime;
    position.x += moveDir.x * moveDistance;
    position.y += moveDir.y * moveDistance;
    position.z += moveDir.z * moveDistance;
}

void Camera::ChangeAngleByInput(float deltaTime)
{
    if (GET(Input).GetKey(VK_LEFT))  yaw -= rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_RIGHT)) yaw += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_UP))    pitch += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_DOWN))  pitch -= rotateSpeed * deltaTime;

    constexpr float MAX_PITCH_DEGREE = 89.0f;   // 90도 찍히면 짐벌락걸려요~
    pitch = max(-MAX_PITCH_DEGREE, min(MAX_PITCH_DEGREE, pitch));
}
