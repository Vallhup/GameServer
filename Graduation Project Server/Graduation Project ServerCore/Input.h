#pragma once

// 0. Server Input / Output 처리
//  - Server Input 처리는 KeyDown/KeyUp Event 기반 Packet 처리
//	 - Intent 기반 처리는 타이밍 동기화 같은 세부 판정 처리가 어려움
//   - All Snapshot 기반 처리는 Network Overhead가 너무 커짐
// 
//  - Server Output 처리는 기존처럼 Intent 기반 Packet 처리
//   - Client에서 Debugging이 좀 힘들어 질 수는 있지만?
//   - 그거 빼고는 단점이 없음
// 
// 1. InputButton 처리는 std::bitset 사용
//  - 정수형 자료형에 BitMask를 직접 씌워서 사용하는 방식은 가독성이 너무 쓰레기임
//  - 개발 편의성 생각해서 bitset 사용하는게 나을 듯
// 
// 2. InputSystem을 Instance 단위로 두어서 Input -> Intent 변환하여 Object의 InputComponent로 전달하여 처리