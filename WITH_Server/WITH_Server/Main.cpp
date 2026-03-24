#include "pch.h"
#include "Framework.h"

int main()
{
	Framework::Get().Start();

	//DBManager& db = DBManager::Get();

	//db.Start(L"WITH_Server_DB");

	//auto cmd = std::make_shared<TestCommand>(1);
	//db.PushCommand(cmd);

	//DBResult result;
	//bool got{ false };

	//auto start = std::chrono::steady_clock::now();
	//while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3))
	//{
	//	if (got = db.TryPopResult(result))
	//		break;

	//	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	//}

	//db.Stop();

	//if (!got)
	//{
	//	std::cout << "No result (timeout)\n";
	//	return -1;
	//}

	//std::wcout
	//	<< L"op=" << (int)result.op
	//	<< L" rid=" << result.requestId
	//	<< L" ok=" << (result.ok ? L"true" : L"false")
	//	<< L" err=" << (int)result.error
	//	<< L" msg=" << result.msg
	//	<< L"\n";

	//return result.ok ? 0 : -1;
}

// 해야할 것들
// 
// 1. Input Buffer + Cancle Window(선택)
//  - Cancle Window는 상황보고 결정
//  - Input Buffer는 웬만해선 하는게 좋긴할듯?
// 
// 2. 게임 관련 스탯, 공식 정리 및 추가
// 
// 3. 시야처리
// 4. 공간분할


// 1. AI FSM / Input Event로 Intent 생성
// 
// 2. Locomotion / Action FSM으로 상태 결정
// 
// 3. 상태에 따라 이동 Contribution 결정
// 
// 4. Contribution Compose하여 FinalMovementDelta 생성
// 
// 5. Movement Apply (Transform 확정)
// 
// 6. Combat Collision Check
// 
// 7. Combat Collision Handling
//
//
// # 위의 루프와 병렬로 
//   Buff 적용, Stat 계산하여 Collision Handling에 적용
// 
// # Stat도 Movement에 적용되는 것도 있고 
//   Collision Handling에 적용되는 것도 있는데?
//   -> 분리 결정
// 
//  # Event Queue의 문제점
//    -> 현재 Event는 동일 프레임에서 처리되는 것도 있고
//       다음 프레임에서 처리되는 것도 있음
// 
//    -> 이를 처리하기 위해 double buffer로는 부족함
// 
//    -> triple buffer로 변경 결정
//