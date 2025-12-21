#pragma once
#include "GameObject.h"

class Camera;

class MainCharacter : public GameObject
{
public:
	void Update(float deltaTime) override;

	void SetCamera(Camera* cam);

public:
	// TEMP : 우선 매 프레임 Key / Mouse Input Check해서 눌려있으면
	//	      Network Packet Send하도록 만들어놨음
	//        추가적으로 현재 클라 예측으로 애니메이션도 실행되도록 해놨는데
	//        나중에 실제 Server에서 Packet 받아서 처리할 때 보정해주는 코드도 필요함
	void BasicMove();
	void BasicAttack();
	void BasicDodge();

private:
	Camera* camera = nullptr;

	XMFLOAT3 characterForward;
	XMFLOAT3 characterRight;

	float currentYawAngle = 0.0f;
	float targetYawAngle = 0.0f;
	bool needsRotation = false;

	int currentAnimState = 0;

	size_t moveSeq = 0;

	static constexpr float MOVE_SPEED = 2.0f;
	static constexpr float ROT_SPEED = 3.14f;
};

