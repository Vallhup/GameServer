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

    centerX = WinSize.x / 2;
    centerY = WinSize.y / 2;

    CURSORINFO cursorInfo;
    cursorInfo.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&cursorInfo);
    bool cursorVisible = (cursorInfo.flags == CURSOR_SHOWING);

    space = cursorVisible;

    ChangeCursorInfo(space);

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
    SetCursor();
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    if (!space)
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
    XMStoreFloat3(&camRight, rightVec);
}

void Camera::ChangeAngleByInput(float deltaTime)
{
    if (GET(Input).GetKey(VK_LEFT))  yaw -= rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_RIGHT)) yaw += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_UP))    pitch += rotateSpeed * deltaTime;
    if (GET(Input).GetKey(VK_DOWN))  pitch -= rotateSpeed * deltaTime;

    POINT mousePos;
    GetCursorPos(&mousePos);

    float deltaX = static_cast<float>(mousePos.x - centerX);
    float deltaY = static_cast<float>(mousePos.y - centerY);

    yaw += deltaX * mouseSensitivity;
    pitch -= deltaY * mouseSensitivity;  

    SetCursorPos(centerX, centerY);

    constexpr float MAX_PITCH_DEGREE = 89.0f;   // 90도 찍히면 짐벌락걸려요~
    pitch = max(-MAX_PITCH_DEGREE, min(MAX_PITCH_DEGREE, pitch));

    UpdateForwardAndRight();
}

XMFLOAT3 Camera::GetForward() const
{
    return camForward;
}

XMFLOAT3 Camera::GetRight() const
{
    return camRight;
}

void Camera::SetCameraPosition(const XMFLOAT3& pos)
{
    targetPosition = { pos.x, pos.y + 2.0f, pos.z };

    // 원래 sqrt(4 * 4 + 2 * 2) 임 임시적으로 실험중
    float distance = sqrt(8 * 8 + 4 * 4); 

    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);

    position.x = targetPosition.x + distance * cos(radPitch) * sin(radYaw);
    position.y = targetPosition.y + distance * sin(radPitch);
    position.z = targetPosition.z + distance * cos(radPitch) * cos(radYaw);
}

void Camera::SetCursor()
{
    if (GET(Input).GetKeyDown(VK_SPACE))
    {
        space = !space;
        ShowCursor(space);

        ChangeCursorInfo(space);

        OutputDebugStringA("space changed!\n");
    }
}

void Camera::ChangeCursorInfo(bool in)
{
    if (in)
        ClipCursor(nullptr);
    else {
        RECT cliprect = { 0, 0, WinSize.x, WinSize.y };
        ClipCursor(&cliprect);
        SetCursorPos(centerX, centerY);
    }
}

void Camera::ReleaseMouse()
{
    ShowCursor(TRUE);

    ClipCursor(nullptr);
}