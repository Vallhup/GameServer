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

void Camera::InitCameraPositionFromCharacter(const XMFLOAT3& pos, float charYawRad)
{
    desiredTargetPos = { pos.x, pos.y + 2.0f, pos.z };

    yaw = XMConvertToDegrees(charYawRad);

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
    if (focusActive)
    {
        UpdateFocusView(core, deltaTime);
        return;
    }

    UpdateInputtoCamLogic(core, deltaTime);
    UpdatePosByObstruction(sceneObjects, instancingBatches, myPlayer);
    UpdateZoomKick(deltaTime);
    UpdateSmoothFollow(deltaTime);
    UpdateShake(deltaTime);
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

    /*if (INPUT.GetKeyDown(VK_F3))
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
    }*/

    /*if (INPUT.GetKeyDown('9'))
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
    }*/
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

    const float focus = GetZoomBell() * ZOOM_KICK_RECENTER;
    XMFLOAT3 orbitCenter;
    XMStoreFloat3(&orbitCenter, XMVectorLerp(XMLoadFloat3(&currentTargetPos), XMLoadFloat3(&desiredTargetPos), focus));

    const float effectiveDistance = currentDistance + GetZoomKick();

    XMFLOAT3 targetCameraPos;
    targetCameraPos.x = orbitCenter.x + effectiveDistance * cos(radPitch) * sin(radYaw);
    targetCameraPos.y = orbitCenter.y + effectiveDistance * sin(radPitch);
    targetCameraPos.z = orbitCenter.z + effectiveDistance * cos(radPitch) * cos(radYaw);

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

    targetPosition = orbitCenter;  
}

void Camera::AddTrauma(float amount)
{
    shakeTrauma = min(1.0f, shakeTrauma + amount);
}

void Camera::UpdateShake(float deltaTime)
{
    if (shakeTrauma <= 0.0f)
        return;

    shakeTime += deltaTime;
    shakeTrauma = max(0.0f, shakeTrauma - SHAKE_DECAY * deltaTime);
}

XMFLOAT3 Camera::GetShakeOffset() const
{
    if (shakeTrauma <= 0.0f)
        return { 0.0f, 0.0f, 0.0f };

    const float shake = shakeTrauma * shakeTrauma * SHAKE_MAX_OFFSET;

    const float t = shakeTime;
    const float nx = sin(t * SHAKE_FREQUENCY) * 0.6f + sin(t * SHAKE_FREQUENCY * 1.7f + 1.3f) * 0.4f;
    const float ny = sin(t * SHAKE_FREQUENCY * 1.1f + 2.1f) * 0.6f + sin(t * SHAKE_FREQUENCY * 2.3f) * 0.4f;

    return {
        camRight.x * nx * shake,
        ny * shake,
        camRight.z * nx * shake
    };
}

void Camera::TriggerDodgeZoom()
{
    zoomKickTime = 0.0f;
}

void Camera::UpdateZoomKick(float deltaTime)
{
    float target = 0.0f;
    if (zoomKickTime >= 0.0f)
    {
        zoomKickTime += deltaTime;
        if (zoomKickTime >= ZOOM_KICK_START && zoomKickTime < ZOOM_KICK_HOLD)
            target = 1.0f;
    }

    const float rate = (target > zoomLevel) ? ZOOM_ATTACK : ZOOM_RELEASE;
    zoomLevel += (target - zoomLevel) * min(rate * deltaTime, 1.0f);

    if (zoomKickTime >= ZOOM_KICK_HOLD && zoomLevel < 0.001f)
    {
        zoomLevel = 0.0f;
        zoomKickTime = -1.0f;
    }
}

float Camera::GetZoomBell() const
{
    return zoomLevel;
}

float Camera::GetZoomKick() const
{
    return ZOOM_KICK_AMPLITUDE * zoomLevel;
}

void Camera::SetCinematicView(DX12Core& core, const XMFLOAT3& eye, const XMFLOAT3& lookAt)
{
    position = eye;
    targetPosition = lookAt;
    currentTargetPos = lookAt;
    desiredTargetPos = lookAt;
    UpdateCameraMatrices(core);
}

void Camera::EnterFocusView(const XMFLOAT3& eye, const XMFLOAT3& lookAt)
{
    focusEye = eye;
    focusLookAt = lookAt;
    focusActive = true;
    focusExiting = false;
}

void Camera::ExitFocusView()
{
    focusExiting = true;
}

void Camera::UpdateFocusView(DX12Core& core, float deltaTime)
{
    const float dir = focusExiting ? -1.0f : 1.0f;
    focusT = clamp(focusT + dir * (deltaTime / FOCUS_DURATION), 0.0f, 1.0f);
    const float s = focusT * focusT * (3.0f - 2.0f * focusT);

    const float radYaw = XMConvertToRadians(yaw);
    const float radPitch = XMConvertToRadians(-pitch);
    const XMFLOAT3 orbitLook = desiredTargetPos;
    const XMFLOAT3 orbitEye{
        orbitLook.x + currentDistance * cos(radPitch) * sin(radYaw),
        orbitLook.y + currentDistance * sin(radPitch),
        orbitLook.z + currentDistance * cos(radPitch) * cos(radYaw)
    };

    XMStoreFloat3(&position, XMVectorLerp(XMLoadFloat3(&orbitEye), XMLoadFloat3(&focusEye), s));
    XMStoreFloat3(&targetPosition, XMVectorLerp(XMLoadFloat3(&orbitLook), XMLoadFloat3(&focusLookAt), s));
    currentTargetPos = targetPosition;

    UpdateCameraMatrices(core);

    if (focusExiting && focusT <= 0.0f)
        focusActive = false;
}

void Camera::UpdateCameraMatrices(DX12Core& core)
{
    const XMFLOAT3 shakeOffset = GetShakeOffset();
    const XMVECTOR shake = XMLoadFloat3(&shakeOffset);

    XMVECTOR eyePos = XMVectorAdd(XMLoadFloat3(&position), shake);
    XMVECTOR lookAt = XMVectorAdd(XMLoadFloat3(&targetPosition), shake);

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
    frameData.screenBrightness = screenBrightness;

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
    RECT cr; GetClientRect(hwnd, &cr);
    POINT center = { (cr.right - cr.left) / 2, (cr.bottom - cr.top) / 2 };
    ClientToScreen(hwnd, &center);

    POINT mousePos;
    GetCursorPos(&mousePos);

    float deltaX = static_cast<float>(mousePos.x - center.x);
    float deltaY = static_cast<float>(mousePos.y - center.y);

    if (abs(deltaX) > 0.1f || abs(deltaY) > 0.1f) {
        yaw += deltaX * mouseSensitivity;
        pitch -= deltaY * mouseSensitivity;

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

        POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
}

void Camera::ReleaseMouse()
{
    ShowCursor(TRUE);

    ClipCursor(nullptr);
}