#include "pch.h"
#include "StartScene.h"
#include "Timer.h"
#include "Input.h"
#include "Indexes.h"
#include "SceneManager.h"
#include "UploadBuffer.h"

void StartScene::Release()
{
	for (const auto& explosion : sExplosions)
		sWorld.RemoveChild(explosion.get());

	sExplosions.clear();
	explosionDirections.clear();
}

void StartScene::Reset()
{
	bExploding = false;
	explosionTime = 0.0f;

	for (int i = 0; i < 3; ++i)
		sName[i].GetTransform().SetScale({ 2.0f, 2.0f, 2.0f });
}


const float* StartScene::GetBackgroundColor()
{
	return color;
}

void StartScene::InitializeProjection()
{
	GET(Graphics).SetProjection(mProjection, 5.0f, 5.0f * (static_cast<float>(WinSize.y) / WinSize.x), 0.01f, 100.0f);
}

void StartScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	InitializeMesh(device, cmdList);
	InitializeLetters();
	mCamera.Initialize(&sCenter, STSCENE_OFFSET, XMVectorSet(91.0f, 180.0f, 1.0f, 0.0f));
}

void StartScene::UpdateLogic(const float deltaTime)
{
	UpdateTitleRotation(deltaTime);
	UpdateNameRotation(deltaTime);

	if (not bExploding)
	{
		if (HandleMouseOverAndClick()) {
			bExploding = true;
			explosionTime = 0.0f;
		}
	}
	else {
		UpdateExplosion(deltaTime);
	}
}

void StartScene::GenerateExplosion(const std::vector<XMFLOAT3>& origins) {
	sExplosions.clear();
	explosionDirections.clear();

	for (const auto& origin : origins) {
		for (int i = 0; i < 25; ++i) {  
			auto obj = std::make_unique<GameObject>();
			Transform& tf = obj->GetTransform();
			tf.SetPosition(origin);
			tf.SetRotation({ static_cast<float>(rand() % 360), static_cast<float>(rand() % 360), static_cast<float>(rand() % 360) });
			tf.SetScale({ 0.5f, 0.5f, 0.5f });
			obj->SetMesh(ColoredCube.get());

			explosionDirections.push_back(RandomDirection());
			sWorld.AddChild(obj.get());
			sExplosions.push_back(std::move(obj));
		}
	}
}

XMFLOAT3 StartScene::RandomDirection()
{
	return {
		(rand() % 200 - 100) / 100.0f,
		(rand() % 200 - 100) / 100.0f,
		(rand() % 200 - 100) / 100.0f
	};
}

const GameObject* StartScene::GetWorld() const
{
	return &sWorld;
}

int StartScene::GetSceneWidth() const
{
	return START_WIDTH;
}

void StartScene::InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& n : Name)
		n = make_unique<VertexIndexBuffer>();
	
	for (auto& t : Title)
		t = make_unique<VertexIndexBuffer>();

	Cube = make_unique<VertexIndexBuffer>();
	ColoredCube = make_unique<VertexIndexBuffer>();

	Name[0]->Initialize(device, cmdList, LetterLEEVertices(0.0f, 0.0f, 1.0f, 1.0f), LetterLEEIndices());
	Name[1]->Initialize(device, cmdList, LetterJEONGVertices(0.0f, 0.0f, 1.0f, 1.0f), LetterJEONGIndices());
	Name[2]->Initialize(device, cmdList, LetterHOVertices(0.0f, 0.0f, 1.0f, 1.0f), LetterHOIndices());
	Title[0]->Initialize(device, cmdList, NumberThreeVertices(1.0f, 0.0f, 0.0f, 1.0f), NumberThreeIndices());
	Title[1]->Initialize(device, cmdList, LetterDVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterDIndices());
	Title[2]->Initialize(device, cmdList, LetterGEVertices(0.0f, 1.0f, 0.0f, 1.0f), LetterGEIndices());
	Title[3]->Initialize(device, cmdList, LetterIMVertices(0.0f, 1.0f, 0.0f, 1.0f), LetterIMIndices());
	Title[4]->Initialize(device, cmdList, LetterPEUVertices(0.0f, 1.0f, 1.0f, 1.0f), LetterPEUIndices());
	Title[5]->Initialize(device, cmdList, LetterLOVertices(0.0f, 1.0f, 1.0f, 1.0f), LetterLOIndices());
	Title[6]->Initialize(device, cmdList, LetterGEUVertices(0.0f, 1.0f, 1.0f, 1.0f), LetterGEUIndices());
	Title[7]->Initialize(device, cmdList, LetterRAEVertices(0.0f, 1.0f, 1.0f, 1.0f), LetterRAEIndices());
	Title[8]->Initialize(device, cmdList, LetterMINGVertices(0.0f, 1.0f, 1.0f, 1.0f), LetterMINGIndices());
	Title[9]->Initialize(device, cmdList, NumberOneVertices(0.4f, 0.2f, 1.0f, 1.0f), NumberOneIndices());
	Cube->Initialize(device, cmdList, CubeVertices(1.0f, 1.0f, 1.0f, 1.0f), CubeIndices());
	ColoredCube->Initialize(device, cmdList, ExplosionCubeVertices(), ExplosionCubeIndices());
}

void StartScene::InitializeLetters()
{
	SetupObject(sTitle[0], { -2.05f, -0.5f, 1.0f }, { 1.5f, 1.5f, 1.5f }, Title[0].get());	
	SetupObject(sTitle[1], { -1.65f, -0.5f, 1.0f }, { 1.5f, 1.5f, 1.5f }, Title[1].get());

	for (int i = 2; i < 4; ++i)
		SetupObject(sTitle[i], { -1.05f + 0.4f * (i - 2), -0.5f, 1.0f }, { 1.5f, 1.5f, 1.5f }, Title[i].get());

	for (int i = 4; i < 9; ++i)
		SetupObject(sTitle[i], { -0.15f + (i - 4) * 0.4f, -0.5f, 1.0f }, { 1.5f, 1.5f, 1.5f }, Title[i].get());

	SetupObject(sTitle[9], { 1.95f, -0.5f, 1.0f }, { 1.5f, 1.5f, 1.5f }, Title[9].get());

	for (int i = 0; i < 3; ++i)
		SetupObject(sName[i], { -0.55f + (i * 0.5f), -0.5f, -0.6f }, { 1.9f, 2.0f, 1.9f }, Name[i].get());
}

void StartScene::SetupObject(GameObject& obj, const XMFLOAT3& pos, const XMFLOAT3& scale, VertexIndexBuffer* mesh)
{
	Transform& tf = obj.GetTransform();
	tf.SetPosition(pos);
	tf.SetScale(scale);
	obj.SetMesh(mesh);
	sWorld.AddChild(&obj);
}

void StartScene::UpdateTitleRotation(float deltaTime)
{
	for (GameObject& obj : sTitle)
	{
		Transform& transform = obj.GetTransform();
		XMFLOAT3 rot = transform.GetRotation();
		rot.x += 45.0f * deltaTime;
		rot.y += 45.0f * deltaTime;
		transform.SetRotation(rot);
	}
}

void StartScene::UpdateNameRotation(float deltaTime)
{
	for (GameObject& obj : sName)
	{
		Transform& transform = obj.GetTransform();
		XMFLOAT3 rot = transform.GetRotation();
		rot.z += 30.0f * deltaTime;
		transform.SetRotation(rot);
	}
}

void StartScene::UpdateExplosion(float deltaTime)
{
	explosionTime += deltaTime;

	for (size_t i = 0; i < sExplosions.size(); ++i) {
		Transform& tf = sExplosions[i]->GetTransform();
		XMVECTOR pos = XMLoadFloat3(&tf.GetPosition());
		XMVECTOR dir = XMLoadFloat3(&explosionDirections[i]);
		pos += dir * deltaTime * 2.0f;
		XMFLOAT3 newPos; XMStoreFloat3(&newPos, pos);
		tf.SetPosition(newPos);
	}

	if (explosionTime >= 1.5f) {
		for (const auto& explosion : sExplosions)
			sWorld.RemoveChild(explosion.get());

		sExplosions.clear();
		explosionDirections.clear();
		GET(SceneManager).ChangeScene(SceneType::Menu);
	}
}

bool StartScene::HandleMouseOverAndClick()
{
	XMFLOAT2 mousePos = GET(Input).GetMousePosition();
	float worldX = (((mousePos.x / WinSize.x) * 2.0f - 1.0f) * 5.0f) / 2.0f;
	float worldZ = ((1.0f - (mousePos.y / WinSize.y) * 2.0f) * 5.0f * (static_cast<float>(WinSize.y) / WinSize.x)) / 2.0f;

	bool mouseOverAny = false;

	for (int i = 0; i < 3; ++i) {
		XMFLOAT3 pos = sName[i].GetTransform().GetPosition();
		float dx = worldX - pos.x;
		float dz = worldZ - pos.z;

		if (dx * dx + dz * dz < 0.2f * 0.2f) {
			mouseOverAny = true;
		}
	}

	if (mouseOverAny && GET(Input).GetMouseButton(MouseButton::LEFT)) {
		std::vector<XMFLOAT3> positions;
		for (auto& obj : sName) {
			positions.push_back(obj.GetTransform().GetPosition());
			obj.GetTransform().SetScale({ 0.0f, 0.0f, 0.0f });
		}
		GenerateExplosion(positions);
		return true;
	}

	return false;
}