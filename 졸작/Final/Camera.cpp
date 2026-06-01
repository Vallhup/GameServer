#include "pch.h"
#include "Camera.h"
#include "GameObject.h"
#include "Input.h"
#include "MainCharacter.h"
#include "InstancingBatch.h"
#include "Mesh.h"
#include "Transform.h"
#include "Terrain.h"
#include "Timer.h"
#include "LookUpTextures.h"

Camera::~Camera()
{
    while (ShowCursor(TRUE) < 0) {}    
    while (ShowCursor(FALSE) >= 0) {}  
    ShowCursor(TRUE);                  
}

void Camera::Initialize(HWND hWnd)
{
    hwnd = hWnd;

	position = { 0.0f, 0.0f, 0.0f };
    targetPosition = { 0.0f, 0.0f, 1.0f };

    desiredPosition = position;
    currentTargetPos = targetPosition;
    desiredTargetPos = targetPosition;

    desiredDistance = currentDistance = zoomDistance = 4.5f;

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

    desiredDistance = currentDistance = zoomDistance = 4.5f;
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
    UpdateInputtoCamLogic(core, deltaTime);
    UpdatePosByObstruction(sceneObjects, instancingBatches, myPlayer);
    UpdateSmoothFollow(deltaTime);
    UpdateCameraMatrices(core);
}

void Camera::UpdateInputtoCamLogic(DX12Core& core, float deltaTime)
{
    if (lutBlendFactor < 1.0f)
        lutBlendFactor = min(lutBlendFactor + deltaTime * lutTransitionSpeed, 1.0f);

    if (!spacePressed)
        ChangeAngleByInput(deltaTime);

    const int wheel = INPUT.GetMouseWheelDelta();
    if (wheel != 0) {
        float steps = (float)wheel / (float)WHEEL_DELTA;
        zoomDistance -= steps * zoomSpeedPerNotch;
        zoomDistance = clamp(zoomDistance, minZoomDistance, maxDistance);
    }

    if (INPUT.GetKeyDown(VK_F3))
    {
        if (lutIndex == 0xFFFFFFFF)
        {
            prevLutIndex = 0;
            lutIndex = 0;
            lutBlendFactor = 1.0f;  
        }
        else if (lutIndex < 219)
        {
            prevLutIndex = lutIndex;
            lutIndex += 1;
            lutBlendFactor = 0.0f;  
        }
        OutputDebugStringA(("lutIndex: " + to_string(lutIndex) + "\n").c_str());
        OutputDebugStringW((L"lutPath: " + core.GetLUTMgr()->GetPathFromIndex(lutIndex) + L"\n").c_str());
    }

    if (INPUT.GetKeyDown(VK_F4))
    {
        if (lutIndex == 0xFFFFFFFF)
        {
            prevLutIndex = 0;
            lutIndex = 0;
            lutBlendFactor = 1.0f;  
        }
        else if (lutIndex > 0)
        {
            prevLutIndex = lutIndex;
            lutIndex -= 1;
            lutBlendFactor = 0.0f;  
        }
        OutputDebugStringA(("lutIndex: " + to_string(lutIndex) + "\n").c_str());
        OutputDebugStringW((L"lutPath: " + core.GetLUTMgr()->GetPathFromIndex(lutIndex) + L"\n").c_str());
    }

    if (INPUT.GetKeyDown(VK_F5))
    {
        prevLutIndex = lutIndex;
        lutIndex = 0xFFFFFFFF;
        lutBlendFactor = 1.0f;  
        OutputDebugStringA(("lutIndex: " + to_string(lutIndex) + "\n").c_str());
    }

    if (INPUT.GetKeyDown('9'))
    {
        if (toneSaturationFactor < 2.0f)
            toneSaturationFactor += 0.1f;
        OutputDebugStringA(("toneSaturationFactor: " + to_string(toneSaturationFactor) + "\n").c_str());
    }

    if (INPUT.GetKeyDown('0'))
    {
        if (toneSaturationFactor > 0.0f)
            toneSaturationFactor -= 0.1f;
        OutputDebugStringA(("toneSaturationFactor: " + to_string(toneSaturationFactor) + "\n").c_str());
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

    if (terrain)
    {
        float minY = terrain->SampleHeightAt(position.x, position.z) + TERRAIN_CLEARANCE;
        if (position.y < minY)
            position.y = minY;
    }

    targetPosition = currentTargetPos;
}

void Camera::SetCinematicView(DX12Core& core, const XMFLOAT3& eye, const XMFLOAT3& lookAt)
{
    position = eye;
    targetPosition = lookAt;
    currentTargetPos = lookAt;
    desiredTargetPos = lookAt;
    UpdateCameraMatrices(core);
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
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 1.0f, 3000.0f);

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
    frameData.time = TIMER.GetTotalTime();
    frameData.lutIndex = lutIndex;
    frameData.prevLutIndex = prevLutIndex;
    frameData.lutBlendFactor = lutBlendFactor;
    frameData.saturationFactor = toneSaturationFactor;

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
    float adjustedDistance = zoomDistance;

    desiredDistance = zoomDistance;

    if (CheckObstruction(sceneObjects, instancingBatches, desiredTargetPos, adjustedDistance, myPlayer))
        desiredDistance = adjustedDistance;
}

bool Camera::CheckObstruction(const vector<shared_ptr<GameObject>>& objects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const XMFLOAT3& targetPos, float& adjustedDistance, const shared_ptr<MainCharacter>& myPlayer)
{
    XMVECTOR rayOrigin = XMLoadFloat3(&targetPos);

    float radYaw = XMConvertToRadians(yaw);
    float radPitch = XMConvertToRadians(-pitch);
    XMFLOAT3 fullPos = {
        targetPos.x + zoomDistance * cos(radPitch) * sin(radYaw),
        targetPos.y + zoomDistance * sin(radPitch),
        targetPos.z + zoomDistance * cos(radPitch) * cos(radYaw)
    };

    XMVECTOR rayDir = XMLoadFloat3(&fullPos) - rayOrigin;
    float maxDistance = zoomDistance;
    rayDir = XMVector3Normalize(rayDir);

    float closestDistance = maxDistance;
    bool foundObstruction = false;

    for (const auto& obj : objects)
    {
        if (obj.get() == myPlayer.get())
            continue;

        TestObjectObstruction(obj, rayOrigin, rayDir, maxDistance, closestDistance, foundObstruction);
    }

    for (const auto& group : instancingBatches)
    {
        for (const auto& obj : group->GetObjects())
            TestObjectObstruction(obj, rayOrigin, rayDir, maxDistance, closestDistance, foundObstruction);
    }

    if (foundObstruction)
    {
        adjustedDistance = max(closestDistance/* - 0.1f*/, minCollisionDistance);
        return true;
    }

    return false;
}

void Camera::TestObjectObstruction(const shared_ptr<GameObject>& obj, FXMVECTOR rayOrigin, FXMVECTOR rayDir, float maxDistance, float& closestDistance, bool& foundObstruction)
{
    if (!obj->ObstructsCamera()) return;

    const BoundingOrientedBox& worldBox = obj->GetWorldBoundingBox();
    if (worldBox.Extents.x <= 0.0f) return;

    float boxDist = 0.0f;
    if (!worldBox.Intersects(rayOrigin, rayDir, boxDist)) return;
    if (boxDist >= maxDistance) return;

    auto* mesh = obj->GetComponent<Mesh>();
    auto* transform = obj->GetComponent<Transform>();

    if (!mesh || !mesh->HasCollisionData() || !transform)
        return;

    XMMATRIX invWorld = XMMatrixInverse(nullptr, transform->GetWorldMatrix());
    XMVECTOR localOrigin = XMVector3TransformCoord(rayOrigin, invWorld);
    XMVECTOR localDir = XMVector3TransformNormal(rayDir, invWorld);

    float localLen = XMVectorGetX(XMVector3Length(localDir));
    if (localLen < 1e-6f) return;

    XMVECTOR localDirUnit = XMVectorScale(localDir, 1.0f / localLen);
    float localMax = maxDistance * localLen;   

    float localHit = 0.0f;
    if (RayTriangleNearest(mesh->GetCollisionPositions(), mesh->GetCollisionIndices(),
        localOrigin, localDirUnit, localMax, localHit))
    {
        float triDist = localHit / localLen;   
        if (triDist >= 0.1f && triDist < closestDistance)
        {
            closestDistance = triDist;
            foundObstruction = true;
        }
    }
}

bool Camera::RayTriangleNearest(const vector<XMFLOAT3>& positions, const vector<UINT>& indices, FXMVECTOR origin, FXMVECTOR dir, float maxDistance, float& outDist)
{
    bool hit = false;
    float nearest = maxDistance;

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        XMVECTOR v0 = XMLoadFloat3(&positions[indices[i]]);
        XMVECTOR v1 = XMLoadFloat3(&positions[indices[i + 1]]);
        XMVECTOR v2 = XMLoadFloat3(&positions[indices[i + 2]]);

        float t = 0.0f;
        if (TriangleTests::Intersects(origin, dir, v0, v1, v2, t))
        {
            if (t > 1e-4f && t < nearest)
            {
                nearest = t;
                hit = true;
            }
        }
    }

    if (hit)
        outDist = nearest;
    return hit;
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

void Camera::SetLutPreset(UINT idx, float saturation)
{
    prevLutIndex = lutIndex;
    lutIndex = idx;
    toneSaturationFactor = saturation;
    lutBlendFactor = 0.0f;
}

void Camera::SetCameraPosition(const XMFLOAT3& pos)
{
    desiredTargetPos = { pos.x, pos.y + 2.0f, pos.z };
}

void Camera::SetCursor(bool in)
{
    spacePressed = in;
    ShowCursor(in);
    ChangeCursorInfo(in);
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