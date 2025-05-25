#include "pch.h"
#include "RCScene.h"
#include "Indexes.h"
#include "Input.h"
#include "SceneManager.h"

void RollerCoster::Release()
{
    for (GameObject* obj : rTrackPieces)
        delete obj;

    rTrackPieces.clear();
}

void RollerCoster::Reset()
{

}

const float* RollerCoster::GetBackgroundColor()
{
    return color;
}

void RollerCoster::InitializeProjection()
{
	GET(Graphics).SetProjection(mProjection, 45.0f, 0.1f, 100.0f);
}

void RollerCoster::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	InitializeMesh(device, cmdList);

    vector<ControlPoint> controlPoints = DefineControlPoints();
    CreateTrackFromControlPoints(controlPoints);

    InitializeCart();
    InitializeRCBody();
    InitializeCamera();
}

void RollerCoster::UpdateLogic(const float deltaTime)
{
    HandleInput();
    UpdateTrackProgress(deltaTime);
    UpdateCartPosition();

    mCamera.Update(deltaTime, RCSCENE_OFFSET, mTrackSpeed + 5.0f);
}

const GameObject* RollerCoster::GetWorld() const
{
	return &rWorld;
}

int RollerCoster::GetSceneWidth() const
{
	return ROLLERCOSTER_GAME_WIDTH;
}

void RollerCoster::InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	/*CreateCubeIndexes(rCube);
    CreateTankBodyIndexes(rCartBody);*/
    cube->Initialize(device, cmdList, RCCubeVertices(), RCCubeIndices());
    body->Initialize(device, cmdList, TankBodyVertices(), TankBodyIndices());
    human->Initialize(device, cmdList, CubeVertices(0.984f, 0.807f, 0.694f, 1.0f), CubeIndices());
    hair->Initialize(device, cmdList, CubeVertices(0.0f, 0.0f, 0.0f, 1.0f), CubeIndices());
}

void RollerCoster::InitializeRCBody()
{
    Transform& transform = rBody.GetTransform();
    transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    rBody.SetMesh(cube.get());
    rWorld.AddChild(&rBody);
}

void RollerCoster::InitializeCart()
{
    {
        Transform& transform = rHumanHair.GetTransform();
        transform.SetPosition({ 0.0f, 0.055f, 0.0f });
        transform.SetRotation({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 1.1f, 0.2f, 1.1f });

        rHumanHair.SetMesh(hair.get());
        rHumanHead.AddChild(&rHumanHair);
    }

    {
        Transform& transform = rHumanNose.GetTransform();
        transform.SetPosition({ 0.0f, 0.0f, 0.065f });
        transform.SetRotation({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 0.1f, 0.1f, 1.0f });

        rHumanNose.SetMesh(human.get());
        rHumanHead.AddChild(&rHumanNose);
    }

    {
        Transform& transform = rHumanHead.GetTransform();
        transform.SetPosition({ 0.0f, 0.15f, 0.0f });
        transform.SetRotation({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 1.5f, 2.0f, 1.5f });

        rHumanHead.SetMesh(human.get());
        rHumanBody.AddChild(&rHumanHead);
    }

    {
        Transform& transform = rHumanBody.GetTransform();
        transform.SetPosition({ 0.0f, 0.0625f, 0.0f });
        transform.SetRotation({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 0.3f, 0.35f, 0.3f });

        rHumanBody.SetMesh(human.get());
        rCart.AddChild(&rHumanBody);
    }

    {
        Transform& transform = rCart.GetTransform();
        transform.SetRotation({ 0.0f, 0.0f, 0.0f });
        transform.SetScale({ 0.7f, 0.5f, 0.7f });

        rCart.SetMesh(body.get());
        rBody.AddChild(&rCart);
    }
}

void RollerCoster::InitializeCamera()
{
    mCamera.Initialize(&rCart, RCSCENE_OFFSET, XMVectorSet(45.0f, 0.0f, 1.0f, 0.0f));
}

void RollerCoster::HandleInput()
{
    if (GET(Input).GetKey(VK_ESCAPE))
    {
        GET(SceneManager).ChangeScene(SceneType::Menu);
        mCurrentTrackIndex = 0;
    }
    if (mCurrentTrackIndex >= rBody.GetChildCount() - 2 || GET(Input).GetKey('N')) {
        GET(SceneManager).ChangeScene(SceneType::Scene2);
        mCurrentTrackIndex = 0;
    }
}

void RollerCoster::UpdateTrackProgress(const float deltaTime)
{
    mTrackProgress += mTrackSpeed * deltaTime;

    while (mTrackProgress >= 1.0f) {
        mTrackProgress -= 1.0f;
        mCurrentTrackIndex++;
    }
}

void RollerCoster::UpdateCartPosition()
{
    const GameObject* currentTrack = rBody.GetChild(mCurrentTrackIndex);
    const GameObject* nextTrack = rBody.GetChild((mCurrentTrackIndex + 1) % rBody.GetChildCount());

    if (not currentTrack || not nextTrack) return;

    const Transform& currentTransform = currentTrack->GetTransform();
    const Transform& nextTransform = nextTrack->GetTransform();

    XMFLOAT3 currentPos = currentTransform.GetPosition();
    XMFLOAT3 nextPos = nextTransform.GetPosition();
    XMFLOAT3 currentRot = currentTransform.GetRotation();
    XMFLOAT3 nextRot = nextTransform.GetRotation();

    NormalizeRotation(currentRot);

    XMFLOAT3 trackPosition = InterpolatePosition(currentPos, nextPos);
    XMFLOAT3 trackRotation = InterpolateRotation(currentRot, nextRot);

    XMFLOAT3 cartPosition = CalculateCartPosition(trackPosition, trackRotation);
    XMFLOAT3 cartRotation = CalculateCartRotation(trackRotation);

    Transform& centerTransform = rCart.GetTransform();
    centerTransform.SetPosition(cartPosition);
    centerTransform.SetRotation(cartRotation);
}

void RollerCoster::NormalizeRotation(XMFLOAT3& rotation)
{
    if (rotation.z > 357.0f) rotation.z = 0.1f;
    if (rotation.y > 357.0f) rotation.y = 0.1f;
}

XMFLOAT3 RollerCoster::InterpolatePosition(const XMFLOAT3& pos1, const XMFLOAT3& pos2)
{
    return {
        pos1.x + (pos2.x - pos1.x) * mTrackProgress,
        pos1.y + (pos2.y - pos1.y) * mTrackProgress,
        pos1.z + (pos2.z - pos1.z) * mTrackProgress
    };
}

XMFLOAT3 RollerCoster::InterpolateRotation(const XMFLOAT3& rot1, const XMFLOAT3& rot2)
{
    return {
        rot1.x + (rot2.x - rot1.x) * mTrackProgress,
        rot1.y + (rot2.y - rot1.y) * mTrackProgress,
        rot1.z + (rot2.z - rot1.z) * mTrackProgress
    };
}

XMFLOAT3 RollerCoster::CalculateCartPosition(const XMFLOAT3& trackPos, const XMFLOAT3& trackRot)
{
    float yOffset = 0.075f;
    float angleRadZ = trackRot.z * XM_PI / 180.0f;
    float angleRadY = trackRot.y * XM_PI / 180.0f;

    return {
        trackPos.x - yOffset * sinf(angleRadZ) * cosf(angleRadY),
        trackPos.y + yOffset * cosf(angleRadZ),
        trackPos.z - yOffset * sinf(angleRadZ) * sinf(angleRadY)
    };
}

XMFLOAT3 RollerCoster::CalculateCartRotation(const XMFLOAT3& trackRot)
{
    return { -trackRot.z, 90.0f + trackRot.y, 0.0f };
}

XMFLOAT3 RollerCoster::CalculateDirection(const XMFLOAT3& start, const XMFLOAT3& end)
{
    return {
        end.x - start.x,
        end.y - start.y,
        end.z - start.z
    };
}

float RollerCoster::CalculateDistance(const XMFLOAT3& direction)
{
    return sqrt(
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z
    );
}

std::vector<ControlPoint> RollerCoster::DefineControlPoints()
{
    std::vector<ControlPoint> controlPoints = {
        { {6.1f, 0.0f, 6.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {5.6f, 0.0f, 5.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {5.1f, 0.0f, 5.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {4.6f, 0.0f, 4.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {4.1f, 0.0f, 4.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {3.6f, 0.0f, 3.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {3.1f, 0.0f, 4.0f}, {0.0f, -270.0f, 0.0f}, true },
        { {2.6f, 0.0f, 4.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {2.1f, 0.0f, 4.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {1.6f, 0.0f, 3.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {1.1f, 0.0f, 3.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {0.6f, 0.0f, 2.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {0.1f, 0.0f, 2.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {-0.4f, 0.0f, 1.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {-0.9f, 0.0f, 1.0f}, {0.0f, -90.0f, 0.0f}, true },
        { {-1.4f, 0.0f, 0.5f}, {0.0f, -180.0f, 0.0f}, true },
        { {-1.4f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, false },
        { {0.2f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, true },
        { {0.6f, 0.4f, 0.1f}, {0.0f, 0.0f, 90.0f}, true },
        { {0.2f, 0.8f, 0.2f}, {0.0f, 0.0f, 180.0f}, true },
        { {-0.2f, 0.4f, 0.3f}, {0.0f, 0.0f, 270.0f}, true },
        { {0.2f, 0.0f, 0.4f}, {0.0f, 0.0f, 360.0f}, false },
        { {1.2f, 0.0f, 0.4f}, {0.0f, 0.0f, 0.0f}, true },
        { {1.6f, 0.4f, 0.5f}, {0.0f, 0.0f, 90.0f}, true },
        { {1.2f, 0.8f, 0.6f}, {0.0f, 0.0f, 180.0f}, true },
        { {0.8f, 0.4f, 0.7f}, {0.0f, 0.0f, 270.0f}, true },
        { {1.2f, 0.0f, 0.8f}, {0.0f, 0.0f, 360.0f}, false },
        { {2.2f, 0.0f, 0.8f}, {0.0f, 0.0f, 0.0f}, true },
        { {2.6f, 0.0f, 0.4f}, {0.0f, -90.0f, 0.0f}, true },
        { {3.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, false },
        { {3.7f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, false },
    };

    return controlPoints;
}

void RollerCoster::CreateTrackFromControlPoints(const std::vector<ControlPoint>& controlPoints)
{
    const float pieceLength = 0.1f;  
    const int arcSegments = 10;   

    for (size_t i = 0; i < controlPoints.size() - 1; i++) {
        const ControlPoint& current = controlPoints[i];
        const ControlPoint& next = controlPoints[i + 1];

        ControlPoint currentAdjusted = current;
        if (currentAdjusted.rotation.z >= 360.0f)
            currentAdjusted.rotation.z -= 360.0f;

        XMFLOAT3 direction = CalculateDirection(currentAdjusted.position, next.position);
        float distance = CalculateDistance(direction);

        int numPieces = static_cast<int>(distance / pieceLength);
        if (numPieces < 1) numPieces = 1;

        if (not current.isCurve) {
            CreateStraightTrack(currentAdjusted, next, numPieces);
        }
        else {
            CreateCurvedTrack(currentAdjusted, next, distance, arcSegments);
        }
    }
}

void RollerCoster::CreateTrackPiece(const XMFLOAT3& position, const XMFLOAT3& rotation)
{
    GameObject* trackObject = new GameObject();
    Transform& transform = trackObject->GetTransform();
    transform.SetPosition(position);
    transform.SetRotation(rotation);
    transform.SetScale({ 1.0f, 1.0f, 1.0f });
    trackObject->SetMesh(cube.get());

    rBody.AddChild(trackObject);
    rTrackPieces.push_back(trackObject);
}

void RollerCoster::CreateStraightTrack(const ControlPoint& current, const ControlPoint& next, int numPieces)
{
    XMFLOAT3 direction = CalculateDirection(current.position, next.position);

    for (int j = 1; j <= numPieces; j++) {
        float ratio = static_cast<float>(j) / numPieces;

        XMFLOAT3 position = {
            current.position.x + direction.x * ratio,
            current.position.y + direction.y * ratio,
            current.position.z + direction.z * ratio
        };

        XMFLOAT3 rotation = {
            current.rotation.x + (next.rotation.x - current.rotation.x) * ratio,
            current.rotation.y + (next.rotation.y - current.rotation.y) * ratio,
            current.rotation.z + (next.rotation.z - current.rotation.z) * ratio
        };

        CreateTrackPiece(position, rotation);
    }
}

void RollerCoster::CreateCurvedTrack(const ControlPoint& current, const ControlPoint& next, float distance, int arcSegments)
{
    float angleYDiff = next.rotation.y - current.rotation.y;
    float angleZDiff = next.rotation.z - current.rotation.z;

    if (abs(angleYDiff) > 0.1f) {
        CreateYAxisCurvedTrack(current, next, distance, angleYDiff, angleZDiff, arcSegments);
    }
    else {
        CreateZAxisCurvedTrack(current, next, distance, angleZDiff, arcSegments);
    }
}

void RollerCoster::CreateYAxisCurvedTrack(const ControlPoint& current, const ControlPoint& next,
    float distance, float angleYDiff, float angleZDiff,
    int arcSegments)
{
    float radiusY = distance / (2 * sinf(angleYDiff * XM_PI / 360.0f));
    if (not isfinite(radiusY) || abs(radiusY) > 100.0f)
        radiusY = distance / 2;

    float angleRadY = current.rotation.y * XM_PI / 180.0f;

    XMFLOAT3 centerY = {
        current.position.x + radiusY * cosf(angleRadY + XM_PIDIV2),
        current.position.y,
        current.position.z + radiusY * sinf(angleRadY + XM_PIDIV2)
    };

    for (int j = 1; j <= arcSegments; j++) {
        float ratio = static_cast<float>(j) / arcSegments;

        float currentAngleY = current.rotation.y + angleYDiff * ratio;
        float currentRadY = currentAngleY * XM_PI / 180.0f;
        float currentAngleZ = current.rotation.z + angleZDiff * ratio;

        XMFLOAT3 position = {
            centerY.x - radiusY * cosf(currentRadY + XM_PIDIV2),
            current.position.y + (next.position.y - current.position.y) * ratio,
            centerY.z - radiusY * sinf(currentRadY + XM_PIDIV2)
        };

        XMFLOAT3 rotation = {
            current.rotation.x + (next.rotation.x - current.rotation.x) * ratio,
            -currentAngleY,
            currentAngleZ
        };

        CreateTrackPiece(position, rotation);
    }
}

void RollerCoster::CreateZAxisCurvedTrack(const ControlPoint& current, const ControlPoint& next,
    float distance, float angleZDiff, int arcSegments)
{
    float radius = distance / (2 * sinf(angleZDiff * XM_PI / 360.0f));
    if (not isfinite(radius) || abs(radius) > 100.0f)
        radius = distance / 2;

    float angleRad = current.rotation.z * XM_PI / 180.0f;
    XMFLOAT3 center = {
        current.position.x + radius * cosf(angleRad + XM_PIDIV2),
        current.position.y + radius * sinf(angleRad + XM_PIDIV2),
        current.position.z
    };

    for (int j = 1; j <= arcSegments; j++) {
        float ratio = static_cast<float>(j) / arcSegments;
        float currentAngleZ = current.rotation.z + angleZDiff * ratio;
        float currentRadZ = currentAngleZ * XM_PI / 180.0f;

        XMFLOAT3 position = {
            center.x - radius * cosf(currentRadZ + XM_PIDIV2),
            center.y - radius * sinf(currentRadZ + XM_PIDIV2),
            current.position.z + (next.position.z - current.position.z) * ratio
        };

        XMFLOAT3 rotation = {
            current.rotation.x + (next.rotation.x - current.rotation.x) * ratio,
            current.rotation.y + (next.rotation.y - current.rotation.y) * ratio,
            currentAngleZ
        };

        CreateTrackPiece(position, rotation);
    }
}