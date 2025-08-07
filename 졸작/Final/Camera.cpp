#include "pch.h"
#include "Camera.h"
#include "Input.h"
#include "DX12Graphics.h"
#include "UploadBuffer.h"

Camera& Camera::Get()
{
    static Camera camera;
    return camera;
}

void Camera::Initialize()
{
	position = { 0.0f, 2.0f, -5.0f };

	yaw = { 0.0f };
	pitch = { 0.0f };
	moveSpeed = { 5.0f };
	rotateSpeed = { 90.0f };

    UpdateForwardAndRight();

    //OutputDebugStringA("Camera init!!\n");
}

void Camera::Update(float deltaTime)
{
    UpdateInputtoCamLogic(deltaTime);
    ApplyToCB();
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    UpdateForwardAndRight();
    ChangeAngleByInput(deltaTime);
    ChangePosByInput(deltaTime);
}

void Camera::ApplyToCB()
{
    XMVECTOR eyePos = XMLoadFloat3(&position);
    XMVECTOR lookAt = XMVectorAdd(eyePos, XMLoadFloat3(&camForward));
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX matView = XMMatrixLookAtLH(eyePos, lookAt, upDir);

    XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 1000.0f);

    matView = XMMatrixTranspose(matView);
    matProj = XMMatrixTranspose(matProj);

    GET(DX12Graphics).GetFrameCB()->CopyData(&matView, sizeof(XMMATRIX), 0);
    GET(DX12Graphics).GetFrameCB()->CopyData(&matProj, sizeof(XMMATRIX), sizeof(XMMATRIX));
}

void Camera::UpdateForwardAndRight()
{
    camForward = {
        cos(XMConvertToRadians(pitch)) * sin(XMConvertToRadians(yaw)),
        sin(XMConvertToRadians(pitch)),
        cos(XMConvertToRadians(pitch)) * cos(XMConvertToRadians(yaw))
    };

    right = { camForward.z, 0.0f, -camForward.x };
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

    pitch = max(-89.0f, min(89.0f, pitch));
}
