#pragma once

#include "Protocols/Protocol.pb.h"

class IGameLogic {
public:
	virtual ~IGameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) = 0;
	virtual void NetworkUpdate() = 0;
	virtual void OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET& packet) = 0;
};

class GameLogic : public IGameLogic {
public:
	GameLogic() = delete;
	GameLogic(Instance* instance);
	virtual ~GameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;
	virtual void NetworkUpdate() override;
	virtual void OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET& packet) override;

private:
	Instance* _instance;
};

// 발전 사항
//
// 1. LogicUpdate Multi-Thread화
//  - Logic Thread, Job-Queue 관리하는 Class 구현 (LogicThreadPool)
//  - LogicUpdate를 Job 형태로 만들어서 LogicThreadPool에 Enqueue하는 형태
//  - 내부적으로 Monster AI (Script 연동되는 부분) 쪽은 별도로 처리할 수도?
//  - LogicThreadPool에서는 매 틱마다 Job-Queue에서 Job 꺼내서 실행
//
// 2. NetworkUpdate 수정
//  - 현재 이동만 구현해놔서 임시로 TransformComonent의 
//    Version Check / Network Packet Send 하고있음
//  - 각 Component에서 자신의 Version 관리하고 그에 맞는 Packet Send 하도록 책임 분리
// 
// 3. Collision Update 추가
//  - Collision은 각 Component에서 처리하기 부자연스러움
//  - GameLogic에 통합 Collision System 만들어서 LogicUpdate에 추가