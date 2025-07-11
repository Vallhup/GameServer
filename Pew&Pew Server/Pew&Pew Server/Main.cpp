#include "pch.h"

// 
// 앞으로 추가해야 할 것들
// 
// Client
// 1. 프로그램 시작할 때 IP 입력받아서 Init
//  - 이건 뭐 어려운거도 아니고 오히려 테스트 할 때는 더 불편해서 천천히 해도 됨
// 
// 2. NetworkManager Init 이후 제대로 연결되면 Rendering & Game Logic 시작 되도록
//  - 지금은 서버 연결 여부 관계 없이 게임 진행 가능함
//  - 연결만 되면 바로 게임 시작 or 따로 Login Packet을 만들거나...
// 
// 3. Recv 자체는 현재 NetworkManager Update에서 처리하고 있는데 이후 패킷 처리는 추가해야 함
//  - 뭘 좀 만져보려 했는데 코드 이해가 안되서 어떻게 수정해야 될 지 모르겠음...
// 
// 4. 모든 입력에 Packet Send 추가
//  - 이건 크게 어렵진 않을 듯
// 
// 5. Object Container 추가
//  - 보통 array or unordered_map 많이 씀 (난 unordered_map만 써봤음 / 싱글스레드에서는 다를거 없음)
//  - Add Packet 받으면 Container에 Character 객체 추가하고 Remove Packet 받으면 객체 지우고...
//  - Rendering도 이 Object Container에 있는 Character 객체 모두 해줘야 함
//
// Server
// 1. Packet 처리 완성
//  - 아직 Dummy Packet 전송만 하는 중
//  - Packet 처리하는 로직 따로 구성해야 됨
// 
// 2. Session 관리 고도화
//  - Client DisConnect 할 때 마다 Send 오류 발생
//  - 큰 문제 있는 상태는 아니지만 그냥 불편함
// 
// 3. Game Logic 추가
//  - 이건 일단 나중에...
//  - PvP를 하면 매칭은 어떻게 할지, 이동 및 충돌처리에 대한 검증 등등...
// 
// 4. GameObject(Character) & Map Data 처리 관련
//  - 말 그대로
//

int main()
{
	setlocale(LC_ALL, "korean");

	Logger::Init("", "C:/Users/Hadenpel/Desktop/GameServer/Pew&Pew Server/Pew&Pew ServerCore/");
	Logger::SetLevel(LogLevel::Debug);

	auto service = std::make_shared<Service>();

	if (not service->Init()) {
		LOG_ERR("Service Init Failed");
		return -1;
	}

	service->Run();
	service->Stop();

	return 0;
}