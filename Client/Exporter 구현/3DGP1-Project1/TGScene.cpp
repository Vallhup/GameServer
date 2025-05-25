#include "pch.h"
#include "TGScene.h"
#include "Input.h"
#include "Timer.h"
#include "Indexes.h"
#include "SceneManager.h"

void TankGame::Release()
{
	/*DeleteEach(mOldBrush);
	DeleteEach(mBrush);
	DeleteEach(mOldPen);
	DeleteEach(mPen);
	DeleteEach(mMemBitmap);
	DeleteEach(mMemDC);

	if (nullptr != mhdc)
	{
		ReleaseDC(mhwnd, mhdc);
		mhdc = nullptr;
	}

	tPlane.Release();
	tCube.Release();
	tBody.Release();
	for (auto& letter : tLetter)
		letter.Release();

	for (auto& enemy : tEnemies)
		enemy.Release(&tWorld);

	for (auto& bullet : tBullets)
		bullet.Release(&tWorld);*/
}

void TankGame::Reset()
{
	tTankBody.GetTransform().SetPosition({ 8.0f, 0.035f, 8.0f });
	tTankBody.GetTransform().SetRotation({ 0.0f, 0.0f, 0.0f });
	tTankHead.GetTransform().SetRotation({ 0.0f, 0.0f, 0.0f });
}

template <typename T>
void TankGame::DeleteEach(T*& member)
{
	if (nullptr != member) {
		DeleteObject(member);
		member = nullptr;
	}
}

const float* TankGame::GetBackgroundColor()
{
	return color;
}

void TankGame::InitializeProjection()
{
	GET(Graphics).SetProjection(mProjection, 60.0f, 0.01f, 300.0f);
}

void TankGame::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	InitializeMesh(device, cmdList);
	InitializeGround();
	InitializeTank();
	InitializeEnemies(device, cmdList, &tWorld);
	InitializeObstacles();
	InitializeBullets(&tWorld, tBullet.get());
	InitializeVicLett();
	tWorld.AddChild(&tMousePoint);
	mCamera.Initialize(&tTankBody, TGSCENE_OFFSET, XMVectorSet(60.0f, -45.0f, 1.0f, 0.0f));
}

void TankGame::UpdateLogic(const float deltaTime)
{
	EndingKeyLogic();
	GetMousePosAndPicking();
	MoveTank(deltaTime);
	UpdateEnemies(deltaTime);
	UpdateTankBodyRotation(deltaTime);
	UpdateTurretRotation(deltaTime);
	UpdateBullets(deltaTime);
	UpdateEnemyBullets();
	ProcessTankFire();
	ToggleShield();

	if (tTankLife <= 0)
		PostQuitMessage(0);

	if (CheckVictory())
		ShowVictoryText();

	mCamera.Update(deltaTime, TGSCENE_OFFSET, 4.0f);
}

const GameObject* TankGame::GetWorld() const
{
	return &tWorld;
}

int TankGame::GetSceneWidth() const
{
	return TANK_GAME_WIDTH;
}

void TankGame::InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& l : tLetter)
		l = make_unique<VertexIndexBuffer>();

	tPlane->Initialize(device, cmdList, PlaneVertices(0.42f, 0.42f, 0.42f, 1.0f), PlaneIndices());
	tCube->Initialize(device, cmdList, CubeVertices(0.0f, 0.0f, 1.0f, 0.1f), CubeIndices());
	tObstacle->Initialize(device, cmdList, CubeVertices(0.2f, 1.0f, 0.2f, 0.2f), CubeIndices());
	tBody->Initialize(device, cmdList, TankBodyVertices(), TankBodyIndices());
	tHead->Initialize(device, cmdList, CubeVertices(0.235f, 0.215f, 0.152f, 1.0f), CubeIndices());
	tBarrel->Initialize(device, cmdList, CubeVertices(0.0f, 0.0f, 0.0f, 1.0f), CubeIndices());
	tBullet->Initialize(device, cmdList, CubeVertices(1.0f, 0.0f, 0.0f, 1.0f), CubeIndices());
	tLetter[0]->Initialize(device, cmdList, LetterYVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterYIndices());
	tLetter[1]->Initialize(device, cmdList, LetterOVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterOIndices());
	tLetter[2]->Initialize(device, cmdList, LetterUVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterUIndices());
	tLetter[3]->Initialize(device, cmdList, LetterWVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterWIndices());
	tLetter[4]->Initialize(device, cmdList, NumberOneVertices(1.0f, 0.0f, 0.0f, 1.0f), NumberOneIndices());
	tLetter[5]->Initialize(device, cmdList, LetterNVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterNIndices());
	tLetter[6]->Initialize(device, cmdList, ExclamMarkVertices(1.0f, 0.0f, 0.0f, 1.0f), ExclamMarkIndices());
}

void TankGame::InitializeGround()
{
	GameObject& object = tGround;
	object.GetTransform().SetPosition({ 8.0f, 0.0f, 8.0f });
	object.GetTransform().SetScale({ 40.0f, 1.0f, 40.0f });

	object.SetMesh(tPlane.get());
	tWorld.AddChild(&object);
}

void TankGame::InitializeTank()
{
	{
		tTankFiringPoint.GetTransform().SetPosition({ 0.0f, 0.0f, 0.1f });
		tTankBarrel.AddChild(&tTankFiringPoint);
	}

	{
		Transform& transform = tTankBarrel.GetTransform();
		transform.SetPosition({ 0.0f, 0.0f, 0.08f });
		transform.SetScale({ 0.3f, 0.5f, 0.8f });

		tTankBarrel.SetMesh(tBarrel.get());
		tTankHead.AddChild(&tTankBarrel);
	}

	{
		Transform& transform = tTankHead.GetTransform();
		transform.SetPosition({ 0.0f, 0.075f, 0.0f });
		transform.SetScale({ 0.4f, 0.5f, 0.6f }); 

		tTankHead.SetMesh(tHead.get());
		tTankBody.AddChild(&tTankHead);
	}

	{
		Transform& transform = tTankBody.GetTransform();
		transform.SetPosition({ 8.0f, 0.03f, 8.0f });
		transform.SetScale({ 1.7f, 0.6f, 2.2f });

		tTankBody.SetMesh(tBody.get());
		tWorld.AddChild(&tTankBody);
	}

	{
		Transform& transform = tTankShield.GetTransform();
		transform.SetScale({ 1.8f, 2.0f, 1.8f });

		tTankShield.SetMesh(tCube.get());
		tTankShield.SetTransparent(true);
		tTankBody.AddChild(&tTankShield);
	}

}

void TankGame::InitializeEnemies(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, GameObject* world)
{
	for (Enemy& Enemies : tEnemies)
		Enemies.Initialize(device, cmdList, world, &tTankBody);
	SetEnemyStartPosition();
}

void TankGame::InitializeBullets(GameObject* world, VertexIndexBuffer* cube)
{
	for (Bullet& Bullets : tBullets)
		Bullets.Initialize(world, cube);
}

void TankGame::InitializeObstacles()
{
	array<array<int, 2>, 10> arr;
	int count = 0;

	while (count < 10)
	{
		bool duplicate = false;
		int x = (rand() % 15) < 8 ? (rand() % 2 + 8) : (rand() % 2 + 11);
		int z = (rand() % 15) < 8 ? (rand() % 2 + 8) : (rand() % 2 + 11);

		for (int j = 0; j < count; ++j)
		{
			if (arr[j][0] == x && arr[j][1] == z) {
				duplicate = true;
				break;
			}
		}

		if (not duplicate)
		{
			arr[count][0] = x;
			arr[count][1] = z;

			GameObject& obj = tObstacles[count];
			obj.GetTransform().SetPosition({ x * 0.8f, 0.1f, z * 0.8f });
			obj.GetTransform().SetScale({ 2.0f, 2.0f, 2.0f });
			obj.SetMesh(tObstacle.get());
			obj.SetTransparent(true);
			tWorld.AddChild(&obj);

			count++;
		}
	}
}

void TankGame::InitializeVicLett()
{
	for (int i = 0; i < 7; ++i)
	{
		Transform& transform = tVictoryLetter[i].GetTransform();
		transform.SetRotation({ -15.0f, -45.0f, 0.0f });
		transform.SetScale({ 1.0f, 1.0f, 1.0f });

		tVictoryLetter[i].SetActive(false);
		tVictoryLetter[i].SetMesh(tLetter[i].get());
		tWorld.AddChild(&tVictoryLetter[i]);
	}
}

void TankGame::SetEnemyStartPosition()
{
	array<array<int, 2>, 20> arr;
	int count{};
	while (count < 20)
	{
		bool duplicate = false;
		int x = (rand() % 15) < 8 ? (rand() % 3 + 6) : (rand() % 3 + 11);
		int z = (rand() % 15) < 8 ? (rand() % 3 + 6) : (rand() % 3 + 11);

		for (int j = 0; j < count; ++j)
		{
			if (arr[j][0] == x && arr[j][1] == z) {
				duplicate = true;
				break;
			}
		}

		if (not duplicate) {
			arr[count][0] = x;
			arr[count][1] = z;

			tEnemies[count].SetPosition({ x * 0.8f, 0.03f, z * 0.8f });
			count++;
		}
	}
}

void TankGame::MoveTank(const float deltaTime)
{
	Transform& tank = tTankBody.GetTransform();
	XMVECTOR pos = tank.GetPositionVec();

	float moveSpeed = 0.25f * deltaTime;

	XMVECTOR forward = tank.GetLookVec();
	XMVECTOR right = XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward);
	right = XMVector3Normalize(right);

	XMVECTOR moveDir = XMVectorZero();

	if (GET(Input).GetKey('W')) 
		moveDir = XMVectorAdd(moveDir, forward);

	if (GET(Input).GetKey('S')) 
		moveDir = XMVectorSubtract(moveDir, forward);

	if (GET(Input).GetKey('A')) 
		moveDir = XMVectorSubtract(moveDir, right);

	if (GET(Input).GetKey('D')) 
		moveDir = XMVectorAdd(moveDir, right);

	if (not XMVector3Equal(moveDir, XMVectorZero()))
	{
		moveDir = XMVector3Normalize(moveDir);
		XMVECTOR newPos = XMVectorAdd(pos, XMVectorScale(moveDir, moveSpeed));
		tank.SetPositionVec(newPos);
	}
}

void TankGame::GetMousePosAndPicking()
{
	const XMVECTOR& camPos = mCamera.GetTransform().GetPositionVec();
	XMFLOAT2 mousePos = GET(Input).GetMousePosition();
	XMVECTOR ray = GET(Graphics).ChangeScreenPointToRay(mView, mProjection, mViewPort, mousePos);

	XMVECTOR rayDir = XMVector3Normalize(ray - camPos);
	float rayLength = XMVectorGetY(camPos) / XMVectorGetX(XMVector3Dot(-rayDir, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

	XMVECTOR toPos = camPos + rayDir * rayLength;
	toPos += XMVectorSet(0.0f, 0.1f, 0.0f, 0.0f);	
	tMousePoint.GetTransform().SetPositionVec(toPos);
}

void TankGame::EndingKeyLogic()
{
	if (GET(Input).GetKey(VK_ESCAPE)) 
	{
		GET(SceneManager).ChangeScene(SceneType::Menu);
	}
}

void TankGame::UpdateEnemies(float deltaTime)
{
	for (Enemy& enemy : tEnemies)
		enemy.Update(&tWorld, deltaTime);
}

void TankGame::UpdateTankBodyRotation(float deltaTime)
{
	if (not GET(Input).GetMouseButton(MouseButton::LEFT))
		return;

	XMVECTOR pos = tTankBody.GetTransform().GetPositionVec();
	XMVECTOR tankDir = tTankBody.GetTransform().GetLookVec();
	XMVECTOR pointingDir = XMVector3NormalizeEst(tMousePoint.GetTransform().GetPositionVec() - pos);

	float angleDiff = GET(Graphics).GetAngleBetweenNormals(tankDir, pointingDir);
	if (XMVectorGetY(XMVector3Cross(tankDir, pointingDir)) < 0.0f)
		angleDiff *= -1.0f;

	float rotSpeed = deltaTime * 2.0f;
	tTankBody.GetTransform().SetRotation({ 0.0f, tTankBody.GetTransform().GetRotation().y + angleDiff * rotSpeed, 0.0f });
}

void TankGame::UpdateTurretRotation(float deltaTime)
{
	XMVECTOR pos = tTankBody.GetTransform().GetPositionVec();
	XMVECTOR currentLookDir = XMVector3TransformNormal(tTankHead.GetTransform().GetLookVec(), tTankBody.GetTransform().CreateBasisMatrix());
	XMVECTOR pointingDir = XMVector3NormalizeEst(tMousePoint.GetTransform().GetPositionVec() - pos);

	Enemy* hovered = nullptr;
	if (GET(Input).GetMouseButton(MouseButton::RIGHT))
	{
		for (Enemy& enemy : tEnemies)
		{
			if (!enemy.IsActive()) continue;
			if (enemy.GetBoundingBox().Contains(tMousePoint.GetTransform().GetPositionVec()) != DirectX::DISJOINT)
			{
				pickedEnemy = &enemy;
				OutputDebugString(L"Picked Enemy: Object selected!\n");
				break;
			}
		}
	}

	if (pickedEnemy && pickedEnemy->IsActive())
	{
		XMVECTOR headLook = currentLookDir;
		XMVECTOR toEnemyDir = XMVector3Normalize(pickedEnemy->GetPositionVec() - tTankBody.GetTransform().GetPositionVec());

		float angleDiff = GET(Graphics).GetAngleBetweenNormals(headLook, toEnemyDir);
		if (XMVectorGetY(XMVector3Cross(headLook, toEnemyDir)) < 0.0f)
			angleDiff *= -1.0f;

		float headTurnSpeed = deltaTime * 20.0f;
		tTankHead.GetTransform().SetRotation({ 0.0f, tTankHead.GetTransform().GetRotation().y + angleDiff * headTurnSpeed, 0.0f });

		if (!pickedEnemy->IsActive())
			pickedEnemy = nullptr;
	}
	else
	{
		float angleDiff = GET(Graphics).GetAngleBetweenNormals(currentLookDir, pointingDir);
		if (XMVectorGetY(XMVector3Cross(currentLookDir, pointingDir)) < 0.0f)
			angleDiff *= -1.0f;

		float rotSpeed = deltaTime * 3.0f;
		tTankHead.GetTransform().SetRotation({ 0.0f, tTankHead.GetTransform().GetRotation().y + angleDiff * rotSpeed, 0.0f });
	}
}

void TankGame::UpdateBullets(float deltaTime)
{
	for (Bullet& bullet : tBullets)
	{
		bullet.Update(deltaTime, 5.0f);
		if (!bullet.IsActive()) continue;

		XMVECTOR bulletPos = bullet.GetPosition();

		for (GameObject& obstacle : tObstacles)
		{
			if (!bullet.IsActive()) break;

			BoundingBox box;
			XMMATRIX world = obstacle.GetTransform().CreateWorldMatrix();
			BoundingBox localBox(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0707f, 0.0707f, 0.0707f));
			localBox.Transform(box, world);

			if (box.Contains(bulletPos) != DirectX::DISJOINT)
			{
				bullet.Deactivate();
				break;
			}
		}

		for (Enemy& enemy : tEnemies)
		{
			if (!enemy.IsActive()) continue;
			if (enemy.GetBoundingBox().Contains(bulletPos) != DirectX::DISJOINT)
			{
				enemy.Deactivate();
				bullet.Deactivate();
				enemy.GenerateExplosion(&tWorld);
				break;
			}
		}
	}
}

void TankGame::UpdateEnemyBullets()
{
	for (Enemy& enemy : tEnemies)
	{
		for (Bullet& bullet : enemy.GetBullets())
		{
			if (!bullet.IsActive()) continue;
			if (GetBoundingBox().Contains(bullet.GetPosition()) != DirectX::DISJOINT)
			{
				bullet.Deactivate();
				OnTankHit();
			}
		}
	}
}

void TankGame::ProcessTankFire()
{
	if (!GET(Input).GetKeyDown(VK_SPACE)) return;

	XMMATRIX world = tTankFiringPoint.GetTransform().CreateWorldMatrix()
		* tTankBarrel.GetTransform().CreateWorldMatrix()
		* tTankHead.GetTransform().CreateWorldMatrix()
		* tTankBody.GetTransform().CreateWorldMatrix();

	XMVECTOR startPos = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), world);
	XMVECTOR lookDir = XMVector3TransformNormal(tTankHead.GetTransform().GetLookVec(), tTankBody.GetTransform().CreateBasisMatrix());

	float rotY = GetAngleBetweenNormals(lookDir, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	if (XMVectorGetY(XMVector3Cross(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), lookDir)) < 0.0f)
		rotY *= -1.0f;

	tBullets[tBulletCount].Fire(startPos, rotY);
	tBulletCount = (tBulletCount + 1) % static_cast<int>(tBullets.size());

	GET(Input).Renew();
}

void TankGame::ToggleShield()
{
	if (!GET(Input).GetKeyDown(VK_TAB)) return;

	tTankShield.SetActive(!tTankShield.GetActive());
	GET(Input).Renew();
}

void TankGame::ShowVictoryText()
{
	for (int i = 0; i < 7; ++i)
	{
		Transform& t = tVictoryLetter[i].GetTransform();
		const Transform& body = tTankBody.GetTransform();
		t.SetPosition({ body.GetPosition().x - 0.6f + i * 0.2f, 0.5f, body.GetPosition().z - 0.6f + i * 0.2f });
		tVictoryLetter[i].SetActive(true);
	}
}

BoundingBox TankGame::GetBoundingBox() const
{
	XMMATRIX world = tTankBody.GetTransform().CreateWorldMatrix();
	BoundingBox localBox(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0527f, 0.3f, 0.067f)); 
	BoundingBox worldBox;
	localBox.Transform(worldBox, world);
	return worldBox;
}

void TankGame::OnTankHit()
{	
	if (not tTankShield.IsActive())
	{
		tTankLife -= 1;

		mCamera.StartShake(0.5f, 0.8f);
	}
	else
	{
		mCamera.StartShake(0.25f, 0.8f);
	}
}

bool TankGame::CheckVictory()
{
	bool victory = true;

	for (Enemy& enemy : tEnemies)
	{
		if (enemy.IsActive())
		{
			victory = false;
			break;
		}
	}

	if (GET(Input).GetKey('V')) 
	{
		for (Enemy& enemy : tEnemies)
		{
			if (enemy.IsActive()) {
				enemy.Deactivate();
				enemy.GenerateExplosion(&tWorld);
			}
		}
		victory = true;
	}

	return victory;
}
