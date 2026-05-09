#include "pch.h"
#include "ParrySparkComponent.h"
#include "Shader.h"
#include "Material.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Transform.h"

PSOType ParrySparkComponent::GetPSOType() const { return PSOType::Spark; }

void ParrySparkComponent::Update(float deltaTime)
{
	auto owner = GetGameObject();
	if (owner)
	{
		auto animMachine = owner->GetComponent<AnimationMachine>();
		auto animator = owner->GetComponent<Animator>();
		auto transform = owner->GetComponent<Transform>();

		bool isParrying = animMachine && animMachine->IsPlaying("Parry");

		if (isParrying && animator && animator->IsInitialized() && transform)
		{
			int currentFrame = animator->GetCurrentFrame();

			if (currentFrame >= 20 && currentFrame <= 22)
			{
				if (!sparkSpawned)
				{
					XMFLOAT3 bonePos = animator->GetBonePosition(45);
					XMVECTOR boneRotQuat = animator->GetBoneRotation(45);
					XMMATRIX worldMat = transform->GetWorldMatrix();
					XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

					XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2);
					XMFLOAT3 playerRot = transform->GetRotation();
					XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);
					XMVECTOR finalRotQuat = XMQuaternionMultiply(offsetRot, boneRotQuat);
					finalRotQuat = XMQuaternionMultiply(finalRotQuat, playerRotQuat);
					XMMATRIX rotMat = XMMatrixRotationQuaternion(finalRotQuat);

					XMVECTOR swordDir = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
					swordDir = XMVector3Normalize(swordDir);

					float bladeLength = 1.0f;
					XMVECTOR tipPos = XMVectorAdd(worldPos, XMVectorScale(swordDir, bladeLength));

					XMFLOAT3 spawnPos;
					XMStoreFloat3(&spawnPos, tipPos);
					Spawn(spawnPos, 64);
					sparkSpawned = true;
				}
			}
			else
			{
				sparkSpawned = false;
			}
		}
		else
		{
			sparkSpawned = false;
		}
	}

	if (particles.empty()) return;

	for (auto& p : particles)
	{
		p.age += deltaTime;

		p.position.x += p.velocity.x * deltaTime;
		p.position.y += p.velocity.y * deltaTime;
		p.position.z += p.velocity.z * deltaTime;

		p.velocity.y -= gravity * 0.5 * deltaTime;

		float drag = powf(0.004f, deltaTime);
		p.velocity.x *= drag;
		p.velocity.y *= drag;
		p.velocity.z *= drag;

		float life = 1.0f - (p.age / maxLifetime);
		if (life < 0.0f) life = 0.0f;
		p.size = particleSize * life;
	}

	particles.erase(
		std::remove_if(particles.begin(), particles.end(),
			[this](const SparkParticle& p) { return p.age >= maxLifetime; }),
		particles.end()
	);
}

void ParrySparkComponent::Spawn(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (particles.size() >= maxElements)
			break;

		SparkParticle p;
		p.position = position;

		float theTa = static_cast<float>(rand()) / RAND_MAX * XM_2PI;
		float phi = static_cast<float>(rand()) / RAND_MAX * XM_PI;
		float speed = sparkSpeed * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);

		p.velocity.x = sinf(phi) * cosf(theTa) * speed;
		p.velocity.y = cosf(phi) * speed;
		p.velocity.z = sinf(phi) * sinf(theTa) * speed;

		p.age = 0.0f;
		p.size = particleSize * (0.7f + static_cast<float>(rand()) / RAND_MAX * 0.3f);

		particles.push_back(p);
	}
}

void ParrySparkComponent::Clear()
{
	particles.clear();
	vertices.clear();
	indices.clear();
}

void ParrySparkComponent::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList)
		textureIndex = Material::RegisterTexture(device, cmdList, path);
}

void ParrySparkComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	if (particles.empty()) return;

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (size_t i = 0; i < particles.size(); ++i)
	{
		const auto& p = particles[i];

		float lifeRatio = 1.0f - (p.age / maxLifetime);
		float alpha = max(0.0f, lifeRatio);

		XMVECTOR particlePos = XMLoadFloat3(&p.position);
		XMVECTOR toCamera = XMVectorSubtract(camPosVec, particlePos);
		toCamera = XMVector3Normalize(toCamera);

		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR right = XMVector3Cross(worldUp, toCamera);
		right = XMVector3Normalize(right);
		XMVECTOR up = XMVector3Cross(toCamera, right);
		up = XMVector3Normalize(up);

		float halfSize = p.size * 0.5f;
		XMVECTOR rightScaled = XMVectorScale(right, halfSize);
		XMVECTOR upScaled = XMVectorScale(up, halfSize);

		XMFLOAT3 corners[4];
		XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(particlePos, rightScaled), upScaled));

		UINT16 baseIdx = static_cast<UINT16>(vertices.size());

		vertices.push_back({ corners[0], {0.0f, 1.0f}, alpha });
		vertices.push_back({ corners[1], {1.0f, 1.0f}, alpha });
		vertices.push_back({ corners[2], {1.0f, 0.0f}, alpha });
		vertices.push_back({ corners[3], {0.0f, 0.0f}, alpha });

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 2);
	}
}
