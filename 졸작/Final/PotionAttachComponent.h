#pragma once
#include "Component.h"

class Animator;
class AnimationMachine;
class Transform;
class GameObject;

class PotionAttachComponent : public Component
{
public:
	void Init() override;
	void Update(float deltaTime) override;

	void SetPotion(const shared_ptr<GameObject>& obj) { potion = obj; }
	void SetBoneIndex(int index) { boneIndex = index; }

	void SetLocalAdjust(const XMMATRIX& m) { XMStoreFloat4x4(&localAdjust, m); }
	void SetAttachOffset(float x, float y, float z) { BuildLocalAdjust(x, y, z); }

private:
	void BuildLocalAdjust(float x, float y, float z);

	Animator* animator = nullptr;
	AnimationMachine* animMachine = nullptr;
	Transform* transform = nullptr;   

	weak_ptr<GameObject> potion;
	int boneIndex = -1;

	XMFLOAT4X4 localAdjust;           

	static constexpr int kVisibleId = -2;   
};
