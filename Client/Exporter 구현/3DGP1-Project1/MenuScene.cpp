#include "pch.h"
#include "MenuScene.h"
#include "Timer.h"
#include "Indexes.h"
#include "Input.h"
#include "SceneManager.h"

void MenuScene::Release()
{
	for (int i = 0; i < 4; ++i)
		mWorld.RemoveChild(&mMenu[i]);

	for (int i = 0; i < 8; ++i)
		mWorld.RemoveChild(&mTutorial[i]);

	for (int i = 0; i < 5; ++i)
		mWorld.RemoveChild(&mStart[i]);

	for (int i = 0; i < 6; ++i)
		mWorld.RemoveChild(&mScene1[i]);

	for (int i = 0; i < 6; ++i)
		mWorld.RemoveChild(&mScene2[i]);

	for (int i = 0; i < 3; ++i)
		mWorld.RemoveChild(&mEnd[i]);
}

void MenuScene::Reset()
{
}

const float* MenuScene::GetBackgroundColor()
{
	return color;
}

void MenuScene::InitializeProjection()
{
	GET(Graphics).SetProjection(mProjection, 5.0f, 5.0f * (static_cast<float>(WinSize.y) / WinSize.x), 0.01f, 100.0f);
}

void MenuScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	InitializeMesh(device, cmdList);

	SetupObjects(mMenu, 4, Menu, { -0.85f, -0.5f,  1.0f }, { 2.0f, 2.0f, 2.0f }, 0.6f);
	SetupObjects(mTutorial, 8, Tutorial, { -2.0f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mScene1, 6, Scene1, { -0.3f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mScene2, 6, Scene2, { 1.1f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mStart, 5, Start, { -1.0f,  -0.5f, -0.7f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mEnd, 3, End, { 0.6f,  -0.5f, -0.7f }, { 0.7f, 0.7f, 0.7f }, 0.17f);

	/*SetupObjects(mMenu, 4, Menu, { -0.85f, -0.5f,  1.0f }, { 2.0f, 2.0f, 2.0f }, 0.6f);
	SetupObjects(mTutorial, 8, Tutorial, { -2.0f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mScene1, 6, Scene1, { -0.3f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mScene2, 6, Scene2, { 1.1f,  -0.5f,  0.1f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mStart, 5, Start, { -1.0f,  -0.5f, -0.7f }, { 0.7f, 0.7f, 0.7f }, 0.17f);
	SetupObjects(mEnd, 3, End, { 0.6f,  -0.5f, -0.7f }, { 0.7f, 0.7f, 0.7f }, 0.17f);*/

	mCamera.Initialize(&mCenter, MNSCENE_OFFSET, XMVectorSet(91.0f, 180.0f, 1.0f, 0.0f));
}

void MenuScene::UpdateLogic(const float deltaTime)
{
	RotateObjects(mTutorial, 8, deltaTime);
	RotateObjects(mStart, 5, deltaTime);
	RotateObjects(mScene1, 6, deltaTime);
	RotateObjects(mScene2, 6, deltaTime);
	RotateObjects(mEnd, 3, deltaTime);

	XMFLOAT2 mousePos = GET(Input).GetMousePosition();
	float worldX = (((mousePos.x / WinSize.x) * 2.0f - 1.0f) * 5.0f) / 2.0f;
	float worldZ = ((1.0f - (mousePos.y / WinSize.y) * 2.0f) * 5.0f * (static_cast<float>(WinSize.y) / WinSize.x)) / 2.0f;

	ProcessSelection(mTutorial, 8, SceneType::Start, worldX, worldZ);
	ProcessSelection(mStart, 5, SceneType::Scene1, worldX, worldZ);
	ProcessSelection(mScene1, 6, SceneType::Scene1, worldX, worldZ);
	ProcessSelection(mScene2, 6, SceneType::Scene2, worldX, worldZ);
	ProcessSelection(mEnd, 3, SceneType::END, worldX, worldZ);
}

const GameObject* MenuScene::GetWorld() const
{
	return &mWorld;
}

int MenuScene::GetSceneWidth() const
{
	return MENU_WIDTH;
}

void MenuScene::InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto& m : Menu)
		m = make_unique<VertexIndexBuffer>();

	for (auto& t : Tutorial)
		t = make_unique<VertexIndexBuffer>();

	for (auto& s : Start)
		s = make_unique<VertexIndexBuffer>();

	for (auto& s1 : Scene1)
		s1 = make_unique<VertexIndexBuffer>();

	for (auto& s2 : Scene2)
		s2 = make_unique<VertexIndexBuffer>();

	for (auto& e : End)
		e = make_unique<VertexIndexBuffer>();

	Menu[0]->Initialize(device, cmdList, LetterMVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterMIndices());
	Menu[1]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices());
	Menu[2]->Initialize(device, cmdList, LetterNVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterNIndices());
	Menu[3]->Initialize(device, cmdList, LetterUVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterUIndices());

	Tutorial[0]->Initialize(device, cmdList, LetterTVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterTIndices());
	Tutorial[1]->Initialize(device, cmdList, LetterUVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterUIndices());
	Tutorial[2]->Initialize(device, cmdList, LetterTVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterTIndices());
	Tutorial[3]->Initialize(device, cmdList, LetterOVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterOIndices());
	Tutorial[4]->Initialize(device, cmdList, LetterRVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterRIndices());
	Tutorial[5]->Initialize(device, cmdList, NumberOneVertices(1.0f, 0.0f, 0.0f, 1.0f), NumberOneIndices());
	Tutorial[6]->Initialize(device, cmdList, LetterAVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterAIndices());
	Tutorial[7]->Initialize(device, cmdList, LetterLVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterLIndices());

	Start[0]->Initialize(device, cmdList, LetterSVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterSIndices());
	Start[1]->Initialize(device, cmdList, LetterTVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterTIndices());
	Start[2]->Initialize(device, cmdList, LetterAVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterAIndices());
	Start[3]->Initialize(device, cmdList, LetterRVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterRIndices());
	Start[4]->Initialize(device, cmdList, LetterTVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterTIndices());

	Scene1[0]->Initialize(device, cmdList, LetterSVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterSIndices());
	Scene1[1]->Initialize(device, cmdList, LetterCVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterCIndices());
	Scene1[2]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices());
	Scene1[3]->Initialize(device, cmdList, LetterNVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterNIndices());
	Scene1[4]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices());
	Scene1[5]->Initialize(device, cmdList, NumberOneVertices(1.0f, 0.0f, 0.0f, 1.0f), NumberOneIndices());

	Scene2[0]->Initialize(device, cmdList, LetterSVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterSIndices());
	Scene2[1]->Initialize(device, cmdList, LetterCVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterCIndices());
	Scene2[2]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices());
	Scene2[3]->Initialize(device, cmdList, LetterNVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterNIndices());
	Scene2[4]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices());
	Scene2[5]->Initialize(device, cmdList, NumberTwoVertices(1.0f, 0.0f, 0.0f, 1.0f), NumberTwoIndices());
	
	End[0]->Initialize(device, cmdList, LetterEVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterEIndices()); 
	End[1]->Initialize(device, cmdList, LetterNVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterNIndices()); 
	End[2]->Initialize(device, cmdList, LetterDVertices(1.0f, 0.0f, 0.0f, 1.0f), LetterDIndices());

	/*CreateENGLetter_M(Menu[0]);
	CreateENGLetter_E(Menu[1]);
	CreateENGLetter_N(Menu[2]);
	CreateENGLetter_U(Menu[3]);

	CreateENGLetter_T(Tutorial[0]);
	CreateENGLetter_U(Tutorial[1]);
	CreateENGLetter_T(Tutorial[2]);
	CreateENGLetter_O(Tutorial[3]);
	CreateENGLetter_R(Tutorial[4]);
	CreateENGLetter_I(Tutorial[5]);
	CreateENGLetter_A(Tutorial[6]);
	CreateENGLetter_L(Tutorial[7]);

	CreateENGLetter_S(Start[0]);
	CreateENGLetter_T(Start[1]);
	CreateENGLetter_A(Start[2]);
	CreateENGLetter_R(Start[3]);
	CreateENGLetter_T(Start[4]);

	CreateENGLetter_S(Scene1[0]);
	CreateENGLetter_C(Scene1[1]);
	CreateENGLetter_E(Scene1[2]);
	CreateENGLetter_N(Scene1[3]);
	CreateENGLetter_E(Scene1[4]);
	CreateNUMLetter_1(Scene1[5]);

	CreateENGLetter_S(Scene2[0]);
	CreateENGLetter_C(Scene2[1]);
	CreateENGLetter_E(Scene2[2]);
	CreateENGLetter_N(Scene2[3]);
	CreateENGLetter_E(Scene2[4]);
	CreateNUMLetter_2(Scene2[5]);

	CreateENGLetter_E(End[0]);
	CreateENGLetter_N(End[1]);
	CreateENGLetter_D(End[2]);*/

	M->Initialize(device, cmdList, CubeVertices(0.0f, 1.0f, 0.0f, 1.0f), CubeIndices());
}

void MenuScene::SetupObjects(GameObject* arr, int count, unique_ptr<VertexIndexBuffer>* meshes, const XMFLOAT3& startPos, const XMFLOAT3& scale, float stepX)
{
	for (int i = 0; i < count; ++i)
	{
		Transform& transform = arr[i].GetTransform();
		transform.SetPosition({ startPos.x + i * stepX, startPos.y, startPos.z });
		transform.SetRotation({ 20.0f, 0.0f , 20.0f });
		transform.SetScale(scale);

		arr[i].SetMesh(meshes[i].get());
		mWorld.AddChild(&arr[i]);
	}
}

void MenuScene::RotateObjects(GameObject* arr, int count, float deltaTime)
{
	for (int i = 0; i < count; ++i)
	{
		Transform& transform = arr[i].GetTransform();
		XMFLOAT3 rot = transform.GetRotation();
		rot.z += 45.0f * deltaTime;
		transform.SetRotation(rot);
	}
}

void MenuScene::ProcessSelection(GameObject* arr, int count, SceneType type, float worldX, float worldZ)
{
	for (int i = 0; i < count; ++i)
	{
		XMFLOAT3 pos = arr[i].GetTransform().GetPosition();
		float dx = worldX - pos.x;
		float dz = worldZ - pos.z;

		if (dx * dx + dz * dz < 0.07f * 0.07f)
		{
			if (GET(Input).GetMouseButton(MouseButton::LEFT))
				GET(SceneManager).ChangeScene(type);
		}
	}
}
