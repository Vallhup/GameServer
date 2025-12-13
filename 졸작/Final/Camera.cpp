#include "pch.h"
#include "Camera.h"
#include "DX12Core.h"
#include "Input.h"

void Camera::Initialize()
{
	position = { 0.0f, 0.0f, 0.0f };
    targetPosition = { 0.0f, 0.0f, 1.0f };

    desiredPosition = position;
    currentTargetPos = targetPosition;
    desiredTargetPos = targetPosition;

    desiredDistance = currentDistance = 4.5f;

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

    spacePressed = cursorVisible;

    ChangeCursorInfo(spacePressed);

    UpdateForwardAndRight();
}

void Camera::InitCameraPositionFromCharacter(const XMFLOAT3& pos)
{
    desiredTargetPos = { pos.x, pos.y + 2.0f, pos.z };

    desiredDistance = currentDistance = 4.5f;
    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);

    desiredPosition.x = desiredTargetPos.x + desiredDistance * cos(radPitch) * sin(radYaw);
    desiredPosition.y = desiredTargetPos.y + desiredDistance * sin(radPitch);
    desiredPosition.z = desiredTargetPos.z + desiredDistance * cos(radPitch) * cos(radYaw);

    position = desiredPosition;
    targetPosition = desiredTargetPos;
    currentTargetPos = desiredTargetPos;
}

void Camera::Update(DX12Core& core, float deltaTime)
{
    UpdateInputtoCamLogic(deltaTime);
    UpdateSmoothFollow(deltaTime);
    UpdateCameraMatrices(core);
    SetCursor();
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    if (!spacePressed)
        ChangeAngleByInput(deltaTime);

    const int wheel = GET(Input).GetMouseWheelDelta();
    if (wheel != 0) {
        float steps = (float)wheel / (float)WHEEL_DELTA; 
        desiredDistance -= steps * zoomSpeedPerNotch;
        desiredDistance = std::clamp(desiredDistance, minDistance, maxDistance);
    }
}

void Camera::UpdateSmoothFollow(float deltaTime)
{
    XMVECTOR currentTarget = XMLoadFloat3(&currentTargetPos);
    XMVECTOR desiredTarget = XMLoadFloat3(&desiredTargetPos);
    XMVECTOR newTarget = XMVectorLerp(currentTarget, desiredTarget, TARGET_FOLLOW_SPEED * deltaTime);
    XMStoreFloat3(&currentTargetPos, newTarget);

    currentDistance = currentDistance + (desiredDistance - currentDistance) * (zoomFollowSpeed * deltaTime);
    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);

    XMFLOAT3 targetCameraPos;
    targetCameraPos.x = currentTargetPos.x + currentDistance * cos(radPitch) * sin(radYaw);
    targetCameraPos.y = currentTargetPos.y + currentDistance * sin(radPitch);
    targetCameraPos.z = currentTargetPos.z + currentDistance * cos(radPitch) * cos(radYaw);

    XMVECTOR currentPos = XMLoadFloat3(&position);
    XMVECTOR targetPos = XMLoadFloat3(&targetCameraPos);
    XMVECTOR newPos = XMVectorLerp(currentPos, targetPos, CAMERA_FOLLOW_SPEED * deltaTime);
    XMStoreFloat3(&position, newPos);

    targetPosition = currentTargetPos;
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

    FrameConstants frameData = {};
    frameData.view = matView;
    frameData.projection = matProj;
    frameData.cameraPosition = position;
    frameData.padding = 0.0f;

    core.GetFrameCB()->CopyData(&frameData, sizeof(FrameConstants));
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
    POINT mousePos;
    GetCursorPos(&mousePos);

    float deltaX = static_cast<float>(mousePos.x - centerX);
    float deltaY = static_cast<float>(mousePos.y - centerY);

    if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
        yaw += deltaX * MOUSE_SENSITIVITY;
        pitch -= deltaY * MOUSE_SENSITIVITY;

        SetCursorPos(centerX, centerY);

        constexpr float MAX_PITCH_DEGREE = 89.0f;
        pitch = max(-MAX_PITCH_DEGREE, min(MAX_PITCH_DEGREE, pitch));

        UpdateForwardAndRight();
    }
}

XMFLOAT3 Camera::GetForward() const
{
    return camForward;
}

XMFLOAT3 Camera::GetRight() const
{
    return camRight;
}

XMFLOAT3 Camera::GetPosition() const
{
    return position;
}

XMFLOAT3 Camera::GetTargetPosition() const
{
    return targetPosition;
}

float Camera::GetRadianYaw() const
{
    return XMConvertToRadians(yaw);
}

float Camera::GetRadianPitch() const
{
    return XMConvertToRadians(pitch);
}

void Camera::SetCameraPosition(const XMFLOAT3& pos)
{
    desiredTargetPos = { pos.x, pos.y + 2.0f, pos.z };
}

void Camera::SetCursor()
{
    if (GET(Input).GetKeyDown(VK_SPACE))
    {
        spacePressed = !spacePressed;
        ShowCursor(spacePressed);

        ChangeCursorInfo(spacePressed);

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