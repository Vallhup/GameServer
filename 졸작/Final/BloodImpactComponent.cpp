#include "pch.h"
#include "BloodImpactComponent.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Material.h"

PSOType BloodImpactComponent::GetPSOType() const { return PSOType::Blood; }

void BloodImpactComponent::InitializeBlood(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT maxElems)
{
	Initialize(device, maxElems);

	for (int i = 0; i < 3; ++i)
	{
		slotCBuffers[i] = make_unique<UploadBuffer>();
		slotCBuffers[i]->Initialize(device, sizeof(EffectConstants));
	}

	if (cmdList)
	{
		texIndices[0] = Material::RegisterTexture(device, cmdList, L"../Assets/Effects/Textures/Blood_1_A.png");
		texIndices[1] = Material::RegisterTexture(device, cmdList, L"../Assets/Effects/Textures/Blood_2_A.png");
		texIndices[2] = Material::RegisterTexture(device, cmdList, L"../Assets/Effects/Textures/Blood_3_A.png");
	}

	if (maxLifetime <= 0.0f)
		maxLifetime = 0.7f;
	if (effectColor.x == 1.0f && effectColor.y == 1.0f && effectColor.z == 1.0f)
		effectColor = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void BloodImpactComponent::Update(float deltaTime)
{
	if (instances.empty()) return;

	const float drag = powf(0.5f, deltaTime / max(0.001f, dragHalfLife));

	for (auto& inst : instances)
	{
		inst.age += deltaTime;
		if (inst.age < 0.0f) continue;

		inst.position.x += inst.velocity.x * deltaTime;
		inst.position.y += inst.velocity.y * deltaTime;
		inst.position.z += inst.velocity.z * deltaTime;

		inst.velocity.x *= drag;
		inst.velocity.y *= drag;
		inst.velocity.z *= drag;

		inst.velocity.y -= gravity * deltaTime;
	}

	instances.erase(
		remove_if(instances.begin(), instances.end(),
			[this](const BloodInstance& i) { return i.age >= maxLifetime; }),
		instances.end()
	);
}

void BloodImpactComponent::Spawn(const XMFLOAT3& impactPos, const XMFLOAT3& impactDir)
{
	XMVECTOR dir = XMLoadFloat3(&impactDir);
	if (XMVector3LengthSq(dir).m128_f32[0] < 1e-6f)
		dir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	dir = XMVector3Normalize(dir);

	XMVECTOR upHint = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	if (fabsf(XMVectorGetY(dir)) > 0.95f)
		upHint = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR side = XMVector3Normalize(XMVector3Cross(dir, upHint));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(side, dir));

	auto frand = []() { return static_cast<float>(rand()) / RAND_MAX; };
	auto frandSigned = [&]() { return frand() * 2.0f - 1.0f; };

	for (int slot = 0; slot < 3; ++slot)
	{
		for (int n = 0; n < countPerSlot; ++n)
		{
			if (instances.size() >= maxElements) break;

			float coneR = frand() * 0.18f;
			float coneAngle = frand() * XM_2PI;
			XMVECTOR jitter = XMVectorScale(side, cosf(coneAngle) * coneR);
			jitter = XMVectorAdd(jitter, XMVectorScale(up, sinf(coneAngle) * coneR));
			XMVECTOR launchDir = XMVector3Normalize(XMVectorAdd(dir, jitter));

			float speed = spawnSpeed * (0.9f + frand() * 0.2f);
			XMVECTOR vel = XMVectorScale(launchDir, speed);

			XMVECTOR posOffset = XMVectorScale(side, frandSigned() * 0.04f);
			posOffset = XMVectorAdd(posOffset, XMVectorScale(up, frandSigned() * 0.04f));
			XMVECTOR pos = XMVectorAdd(XMLoadFloat3(&impactPos), posOffset);

			BloodInstance inst;
			XMStoreFloat3(&inst.position, pos);
			XMStoreFloat3(&inst.velocity, vel);
			inst.age = -static_cast<float>(slot) * 0.02f;
			inst.size = baseSize * (0.85f + frand() * 0.3f);
			inst.rotation = frand() * XM_2PI;
			inst.texSlot = static_cast<UINT>(slot);

			instances.push_back(inst);
		}
	}
}

void BloodImpactComponent::Clear()
{
	instances.clear();
	vertices.clear();
	indices.clear();
}

void BloodImpactComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();
	for (int i = 0; i < 3; ++i) { slotIndexStart[i] = 0; slotIndexCount[i] = 0; }

	if (instances.empty()) return;

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (int slot = 0; slot < 3; ++slot)
	{
		UINT startIdx = static_cast<UINT>(indices.size());
		slotIndexStart[slot] = startIdx;

		for (const auto& inst : instances)
		{
			if (inst.texSlot != static_cast<UINT>(slot)) continue;
			if (inst.age < 0.0f) continue;

			float lifeRatio = inst.age / maxLifetime;
			if (lifeRatio >= 1.0f) continue;

			int frame = static_cast<int>(lifeRatio * FLIPBOOK_FRAMES);
			if (frame >= FLIPBOOK_FRAMES) frame = FLIPBOOK_FRAMES - 1;
			int col = frame % FLIPBOOK_COLS;
			int row = frame / FLIPBOOK_COLS;

			float uStep = 1.0f / static_cast<float>(FLIPBOOK_COLS);
			float vStep = 1.0f / static_cast<float>(FLIPBOOK_ROWS);
			float u0 = col * uStep;
			float v0 = row * vStep;
			float u1 = u0 + uStep;
			float v1 = v0 + vStep;

			float alpha = 1.0f;
			if (lifeRatio > 0.75f)
				alpha = 1.0f - (lifeRatio - 0.75f) / 0.25f;

			XMVECTOR particlePos = XMLoadFloat3(&inst.position);
			XMVECTOR toCamera = XMVector3Normalize(XMVectorSubtract(camPosVec, particlePos));

			XMVECTOR velVec = XMLoadFloat3(&inst.velocity);
			float speedSq = XMVectorGetX(XMVector3LengthSq(velVec));

			XMVECTOR axisR, axisU;
			float stretch = 1.0f;
			if (speedSq > 0.01f)
			{
				XMVECTOR velDir = XMVector3Normalize(velVec);
				axisR = XMVector3Normalize(XMVector3Cross(velDir, toCamera));
				axisU = velDir;
				stretch = stretchFactor;
			}
			else
			{
				XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				if (fabsf(XMVectorGetY(toCamera)) > 0.99f)
					worldUp = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
				XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, toCamera));
				XMVECTOR up = XMVector3Normalize(XMVector3Cross(toCamera, right));
				float cosR = cosf(inst.rotation);
				float sinR = sinf(inst.rotation);
				axisR = XMVectorAdd(XMVectorScale(right, cosR), XMVectorScale(up, sinR));
				axisU = XMVectorAdd(XMVectorScale(right, -sinR), XMVectorScale(up, cosR));
			}

			float halfR = inst.size * 0.5f;
			float halfU = inst.size * 0.5f * stretch;
			XMVECTOR rScaled = XMVectorScale(axisR, halfR);
			XMVECTOR uScaled = XMVectorScale(axisU, halfU);

			XMFLOAT3 corners[4];
			XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(particlePos, rScaled), uScaled));
			XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(particlePos, rScaled), uScaled));
			XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(particlePos, rScaled), uScaled));
			XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(particlePos, rScaled), uScaled));

			UINT16 baseIdx = static_cast<UINT16>(vertices.size());

			vertices.push_back({ corners[0], { u0, v1 }, alpha });
			vertices.push_back({ corners[1], { u1, v1 }, alpha });
			vertices.push_back({ corners[2], { u1, v0 }, alpha });
			vertices.push_back({ corners[3], { u0, v0 }, alpha });

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 3);
			indices.push_back(baseIdx + 2);
		}

		slotIndexCount[slot] = static_cast<UINT>(indices.size()) - startIdx;
	}
}

void BloodImpactComponent::Render(DX12Core& core, const XMFLOAT3& cameraPos)
{
	BuildMesh(cameraPos);

	if (vertices.empty() || indices.empty()) return;

	vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(EffectVertex));
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(EffectVertex));
	vbView.StrideInBytes = sizeof(EffectVertex);

	indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
	ibView.Format = DXGI_FORMAT_R16_UINT;

	auto cmdList = core.GetGraphicsCmdList();
	cmdList->SetPipelineState(core.GetShader()->GetPSO(GetPSOType()));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	for (int slot = 0; slot < 3; ++slot)
	{
		if (slotIndexCount[slot] == 0) continue;

		EffectConstants c{};
		c.color = effectColor;
		c.textureIndex = texIndices[slot];
		slotCBuffers[slot]->CopyData(&c, sizeof(EffectConstants));

		cmdList->SetGraphicsRootConstantBufferView(23, slotCBuffers[slot]->GetGPUVirtualAddress());
		cmdList->DrawIndexedInstanced(slotIndexCount[slot], 1, slotIndexStart[slot], 0, 0);
	}
}
