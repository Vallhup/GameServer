#include "pch.h"
#include "GimmickDiamond.h"
#include "Mesh.h"
#include "Material.h"
#include "Transform.h"
#include "DissolveComponent.h"
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

	const XMFLOAT4 ringColor = { color.x * RING_SHADE, color.y * RING_SHADE, color.z * RING_SHADE, 0.0f };
	const XMFLOAT4 apexColor = { color.x * APEX_SHADE, color.y * APEX_SHADE, color.z * APEX_SHADE, 1.0f };

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
			v.uv = { p.x * 0.5f + 0.5f, p.z * 0.5f + 0.5f };	
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
	curHp = packet.curhp();
	maxHp = packet.maxhp();

	syncedPos = { packet.x(), packet.y(), packet.z() };

	if (auto transform = GetComponent<Transform>())
	{
		const float radius = packet.radius();
		transform->SetInitPosition(syncedPos);
		transform->SetScale(radius, radius, radius);
	}

	auto mesh = GetComponent<Mesh>();

	switch (packet.state())
	{
	case Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED:
		prevHp = maxHp;
		shakeTimer = 0.0f;
		markedForRemoval = false;
		if (mesh) mesh->SetBrightness(1.0f);	
		if (auto* dis = GetComponent<DissolveComponent>()) dis->Reset();
		break;

	case Protocol::BOSS_GIMMICK_OBJECT_STATE_UPDATED:
		if (curHp < prevHp)	
		{
			shakeTimer = SHAKE_DURATION;

			const float ratio = (maxHp > 0) ? static_cast<float>(curHp) / maxHp : 0.0f;
			if (mesh) mesh->SetBrightness(APEX_MIN_BRIGHT + (1.0f - APEX_MIN_BRIGHT) * ratio);
		}
		prevHp = curHp;
		break;

	case Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN:
		shakeTimer = 0.0f;
		if (auto* dis = GetComponent<DissolveComponent>()) dis->Start();	
		break;

	case Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED:
		markedForRemoval = true;	
		break;

	default:
		break;
	}

	state = static_cast<int>(packet.state());
}

bool GimmickDiamond::ShouldRemove() const
{
	if (markedForRemoval) return true;
	auto* dis = GetComponent<DissolveComponent>();
	return dis && dis->IsFinished();
}

void GimmickDiamond::Update(float deltaTime)
{
	spin += deltaTime * 0.6f;

	XMFLOAT3 pos = syncedPos;
	if (shakeTimer > 0.0f)
	{
		shakeTimer -= deltaTime;
		const float k = (shakeTimer > 0.0f) ? (shakeTimer / SHAKE_DURATION) : 0.0f;
		const float amp = SHAKE_MAG * k;
		auto jitter = [amp]() { return (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * amp; };
		pos.x += jitter();
		pos.y += jitter();
		pos.z += jitter();
	}

	if (auto transform = GetComponent<Transform>())
	{
		transform->SetInitPosition(pos);
		transform->SetRotation(0.0f, spin, 0.0f);
	}

	GameObject::Update(deltaTime);
}
