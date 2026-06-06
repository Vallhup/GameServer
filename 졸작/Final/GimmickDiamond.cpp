#include "pch.h"
#include "GimmickDiamond.h"
#include "Mesh.h"
#include "Material.h"
#include "Transform.h"
#include "NetId.h"

void GimmickDiamond::Init(DX12Core& core, const XMFLOAT4& color)
{
	AddComponent<Mesh>();
	AddComponent<Transform>();

	BuildDiamondMesh(core, color);
}

void GimmickDiamond::BuildDiamondMesh(DX12Core& core, const XMFLOAT4& color)
{
	const float r = 1.0f;	
	const float h = 1.4f;	

	const XMFLOAT3 top{ 0.0f,  h, 0.0f };
	const XMFLOAT3 bottom{ 0.0f, -h, 0.0f };
	const XMFLOAT3 ring[4] = {
		{  r, 0.0f, 0.0f },
		{ 0.0f, 0.0f,  r },
		{ -r, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -r },
	};

	vector<Vertex> vertices;
	vector<UINT> indices;
	vertices.reserve(24);
	indices.reserve(24);

	const XMFLOAT4 ringColor = { color.x * 0.25f, color.y * 0.25f, color.z * 0.25f, color.w };
	const XMFLOAT4 apexColor = { color.x * 0.9f, color.y * 0.9f, color.z * 0.9f, color.w };

	auto addFace = [&](const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c)
	{
		XMVECTOR va = XMLoadFloat3(&a);
		XMVECTOR vb = XMLoadFloat3(&b);
		XMVECTOR vc = XMLoadFloat3(&c);

		XMVECTOR n = XMVector3Normalize(XMVector3Cross(vb - va, vc - va));
		XMVECTOR center = (va + vb + vc) / 3.0f;
		if (XMVectorGetX(XMVector3Dot(n, center)) < 0.0f)
			n = -n;

		XMFLOAT3 nf;
		XMStoreFloat3(&nf, n);

		for (const XMFLOAT3& p : { a, b, c })
		{
			Vertex v{};
			v.pos = p;
			v.uv = { 0.0f, 0.0f };
			v.normal = nf;
			v.tangent = { 1.0f, 0.0f, 0.0f };
			v.weights = { 0.0f, 0.0f, 0.0f, 0.0f };
			v.indices = { 0.0f, 0.0f, 0.0f, 0.0f };
			v.color = (p.y > -1e-3f && p.y < 1e-3f) ? ringColor : apexColor;

			indices.push_back(static_cast<UINT>(vertices.size()));
			vertices.push_back(v);
		}
	};

	for (int i = 0; i < 4; ++i)
	{
		const XMFLOAT3& a = ring[i];
		const XMFLOAT3& b = ring[(i + 1) % 4];
		addFace(top, a, b);		
		addFace(bottom, b, a);	
	}

	auto mesh = GetComponent<Mesh>();
	mesh->SetProceduralMesh(core, vertices, indices);
	mesh->SetUnlit(true);		
	mesh->SetTwoSided(true);	
}

void GimmickDiamond::SyncFrom(const Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET& packet)
{
	bossNetId = NetId{ packet.bossnetid() }.GetId();
	objectNetId = NetId{ packet.objectnetid() }.GetId();
	gimmickSeq = packet.gimmickseq();
	state = static_cast<int>(packet.state());
	curHp = packet.curhp();
	maxHp = packet.maxhp();

	if (auto transform = GetComponent<Transform>())
	{
		const float radius = packet.radius();
		const float tempLift = 2.0f;	// 임시 오프셋: 서버가 높이 제대로 보내면 제거
		transform->SetInitPosition(packet.x(), packet.y() + tempLift, packet.z());
		transform->SetScale(radius, radius, radius);
	}
}

void GimmickDiamond::Update(float deltaTime)
{
	spin += deltaTime * 0.6f;
	if (auto transform = GetComponent<Transform>())
		transform->SetRotation(0.0f, spin, 0.0f);

	GameObject::Update(deltaTime);
}
