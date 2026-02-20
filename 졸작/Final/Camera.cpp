#include "pch.h"
#include "Camera.h"
#include "DX12Core.h"
#include "Input.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "InstancingBatch.h"

void Camera::Initialize(HWND hWnd)
{
    hwnd = hWnd;

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

void Camera::Update(DX12Core& core, float deltaTime, const vector<shared_ptr<GameObject>>& sceneObjects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const shared_ptr<MainCharacter>& myPlayer)
{
    UpdateInputtoCamLogic(deltaTime);
    UpdatePosByObstruction(sceneObjects, instancingBatches, myPlayer);
    UpdateSmoothFollow(deltaTime);
    UpdateCameraMatrices(core);
    SetCursor();
}

void Camera::UpdateInputtoCamLogic(float deltaTime)
{
    if (!spacePressed)
        ChangeAngleByInput(deltaTime);

    const int wheel = INPUT.GetMouseWheelDelta();
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
    float targetT = min(TARGET_FOLLOW_SPEED * deltaTime, 1.0f);
    XMVECTOR newTarget = XMVectorLerp(currentTarget, desiredTarget, targetT);
    XMStoreFloat3(&currentTargetPos, newTarget);

    float zoomT = min(zoomFollowSpeed * deltaTime, 1.0f);
    currentDistance = currentDistance + (desiredDistance - currentDistance) * zoomT;
    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);

    XMFLOAT3 targetCameraPos;
    targetCameraPos.x = currentTargetPos.x + currentDistance * cos(radPitch) * sin(radYaw);
    targetCameraPos.y = currentTargetPos.y + currentDistance * sin(radPitch);
    targetCameraPos.z = currentTargetPos.z + currentDistance * cos(radPitch) * cos(radYaw);

    XMVECTOR currentPos = XMLoadFloat3(&position);
    XMVECTOR targetPos = XMLoadFloat3(&targetCameraPos);
    float camT = min(CAMERA_FOLLOW_SPEED * deltaTime, 1.0f);
    XMVECTOR newPos = XMVectorLerp(currentPos, targetPos, camT);
    XMStoreFloat3(&position, newPos);

    targetPosition = currentTargetPos;
}

void Camera::UpdateCameraMatrices(DX12Core& core)
{
    XMVECTOR eyePos = XMLoadFloat3(&position);
    XMVECTOR lookAt = XMLoadFloat3(&targetPosition);

    XMVECTOR diff = XMVectorSubtract(lookAt, eyePos);

    float dist = XMVectorGetX(XMVector3Length(diff));
    if (dist < 0.001f)
    {
        lookAt = XMVectorAdd(eyePos, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
    }

    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(eyePos, lookAt, upDir);

    float aspectRatio = static_cast<float>(WinSize.x) / static_cast<float>(WinSize.y);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 50.0f);

    BoundingFrustum::CreateFromMatrix(viewFrustum, proj);

    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    viewFrustum.Transform(viewFrustum, invView);

    XMStoreFloat4x4(&matView, view);
    XMStoreFloat4x4(&matProj, proj);

    view = XMMatrixTranspose(view);
    proj = XMMatrixTranspose(proj);

    FrameConstants frameData = {};
    frameData.view = view;
    frameData.projection = proj;

    XMMATRIX vp = XMMatrixMultiply(XMMatrixTranspose(view), XMMatrixTranspose(proj));
    XMMATRIX invVp = XMMatrixInverse(nullptr, vp);

    frameData.invViewProj = XMMatrixTranspose(invVp);
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
    XMVECTOR rightVec = XMVector3Cross(up, forward);    
    rightVec = XMVector3Normalize(rightVec);
    XMStoreFloat3(&camRight, rightVec);
}

void Camera::ChangeAngleByInput(float deltaTime)
{
    POINT center = { WinSize.x / 2, WinSize.y / 2 };
    ClientToScreen(hwnd, &center);

    POINT mousePos;
    GetCursorPos(&mousePos);

    float deltaX = static_cast<float>(mousePos.x - center.x);
    float deltaY = static_cast<float>(mousePos.y - center.y);

    if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
        yaw += deltaX * MOUSE_SENSITIVITY;
        pitch -= deltaY * MOUSE_SENSITIVITY;

        SetCursorPos(center.x, center.y);

        constexpr float MAX_PITCH_DEGREE = 85.0f;
        pitch = max(-MAX_PITCH_DEGREE, min(MAX_PITCH_DEGREE, pitch));

        UpdateForwardAndRight();
    }
}

void Camera::UpdatePosByObstruction(const vector<shared_ptr<GameObject>>& sceneObjects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const shared_ptr<MainCharacter>& myPlayer)
{
    float adjustedDistance = desiredDistance;

    // Real-time camera position changes
    desiredDistance = maxDistance;

    if (CheckObstruction(sceneObjects, instancingBatches, desiredTargetPos, adjustedDistance, myPlayer))
        desiredDistance = adjustedDistance;
}

bool Camera::CheckObstruction(const vector<shared_ptr<GameObject>>& objects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const XMFLOAT3& targetPos, float& adjustedDistance, const shared_ptr<MainCharacter>& myPlayer)
{
    XMVECTOR rayOrigin = XMLoadFloat3(&targetPos);
    XMVECTOR rayDir = XMLoadFloat3(&position) - rayOrigin;
    
    float maxDistance = XMVectorGetX(XMVector3Length(rayDir));
    rayDir = XMVector3Normalize(rayDir);

    float closestDistance = maxDistance;
    bool foundObstruction = false;

    for (const auto& obj : objects)
    {
        if (obj.get() == myPlayer.get())
            continue;

        BoundingBox worldBox = obj->GetWorldBoundingBox();

        if (worldBox.Extents.x <= 0.0f) continue;

        float distance = 0.0f;
        if (worldBox.Intersects(rayOrigin, rayDir, distance))
        {
            if (distance < 0.1f) continue;
            if (distance >= maxDistance) continue;

            if (distance < closestDistance)
            {
                closestDistance = distance;
                foundObstruction = true;
            }
        }
    }

    for (const auto& group : instancingBatches)
    {
        const auto& batchObjects = group->GetObjects();

        for (const auto& obj : batchObjects)
        {
            BoundingBox worldBox = obj->GetWorldBoundingBox();

            if (worldBox.Extents.x <= 0.0f) continue;

            float distance = 0.0f;
            if (worldBox.Intersects(rayOrigin, rayDir, distance))
            {
                if (distance < 0.1f) continue;
                if (distance >= maxDistance) continue;

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    foundObstruction = true;
                }
            }
        }
    }

    if (foundObstruction)
    {
        adjustedDistance = max(closestDistance/* - 0.1f*/, minDistance);
        return true;
    }

    return false;
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

BoundingFrustum Camera::GetViewFrustum() const
{
    return viewFrustum;
}

XMMATRIX Camera::GetViewMatrix() const
{
    return XMLoadFloat4x4(&matView);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    return XMLoadFloat4x4(&matProj);
}

void Camera::SetCameraPosition(const XMFLOAT3& pos)
{
    desiredTargetPos = { pos.x, pos.y + 2.0f, pos.z };
}

void Camera::SetCursor()
{
    if (INPUT.GetKeyDown(VK_F2))
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
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        POINT topLeft = { clientRect.left, clientRect.top };
        POINT bottomRight = { clientRect.right, clientRect.bottom };
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        RECT clipRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
        ClipCursor(&clipRect);

        POINT center = { WinSize.x / 2, WinSize.y / 2 };
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
}

void Camera::ReleaseMouse()
{
    ShowCursor(TRUE);

    ClipCursor(nullptr);
}