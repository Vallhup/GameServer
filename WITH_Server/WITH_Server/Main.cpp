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