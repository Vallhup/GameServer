#include "pch.h"
#include "Framework.h"

#include "MapCollisionManager.h"

int main()
{
	Framework::Get().Start();
}

// 해야할 것들
//
// 1. ECS, TaskGraph 프레임워크 옮기기
//  - 기본 베이스 코드들 별도 라이브러리로 옮기기
// 
//  - ECS 프레임워크 기능 추가 
//    (JoinView, ComponentStorage Iterator 개선 등)
// 
//  - ECS 프레임워크 성능 측정 및 개선
// 
//  - Intel TBB 적용
// 
// 2. Input Buffer + Cancle Window(선택)
//  - Cancle Window는 상황보고 결정
//  - Input Buffer는 웬만해선 하는게 좋긴할듯?
// 
// 3. 게임 관련 스탯, 공식 정리 및 추가
// 
// 4. 시야처리
// 5. 공간분할