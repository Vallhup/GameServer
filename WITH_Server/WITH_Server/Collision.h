//#pragma once
//
//#include "Collider.h"
////#include "AnimationManager.h"
//
//// 물리 기반 이펙트??
//// ex) 칼에 베인 방향대로 이펙트 출력
////     
//// 1. 칼 휘두른 방향 벡터 계산
////  - 간단하게 생각하면 (현재 프레임 위치 - 이전 프레임 위치)
////  - 이걸 위해선 이전 프레임 위치를 기억하고 있어야 할듯?
//// 
//// 2. 충돌 노멀
////  - 정확하게 뭘 말하는건진 모르겠는데 지금도 바로 구할 수 있다 함
//// 
//// 3. 베어진 방향
////  A. 검의 진행 방향
//// 
////  B. 검 진행 방향을 표면에 투영
////   - 소울라이크 / 몬헌 같은 느낌 난다고 함
////   - 흠... 뭔말인지 모르겠음
//// 
////  C. 충돌 노멀을 기준으로 양쪽 방향 결정
////   - 우리 알빠는 아닌듯
////   - 이거도 뭔말인지 잘 모르겠음
//
//using namespace DirectX;
//
//inline double Dot3(XMVECTOR a, XMVECTOR b)
//{
//	return XMVectorGetX(XMVector3Dot(a, b));
//}
//
//inline double Clamp01(double x)
//{
//	return std::clamp<double>(x, 0.0, 1.0);
//}
//
//struct CapsuleView {
//	XMFLOAT3 p0;
//	XMFLOAT3 p1;
//	double radius;
//};
//
//inline CapsuleView MakeCapsuleView(const CombatCollider& collider, size_t i)
//{
//	CapsuleView out;
//	out.p0 = collider.worldDatas[i].p0;
//	out.p1 = collider.worldDatas[i].p1;
//	out.radius = (*collider.staticDatas)[i].radius;
//
//	return out;
//}
//
//inline CapsuleView MakeCapsuleView(const XMFLOAT3& center, const XMFLOAT3& dir,
//	double halfHeight, double radius)
//{
//	CapsuleView out;
//	out.radius = radius;
//
//	XMVECTOR c = XMLoadFloat3(&center);
//	XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
//
//	XMVECTOR offset = XMVectorScale(d, halfHeight);
//	
//	XMStoreFloat3(&out.p0, XMVectorSubtract(c, offset));
//	XMStoreFloat3(&out.p1, XMVectorAdd(c, offset));
//
//	return out;
//}
//
//namespace Collision {
//	static double SegmentSegmentDistSq(XMVECTOR p1, XMVECTOR q1, XMVECTOR p2, XMVECTOR q2)
//	{
//		constexpr double EPS = 1e-8f;
//
//		XMVECTOR d1 = q1 - p1; // 방향1
//		XMVECTOR d2 = q2 - p2; // 방향2
//		XMVECTOR r = p1 - p2;
//
//		double a = Dot3(d1, d1); // |d1|^2
//		double e = Dot3(d2, d2); // |d2|^2
//		double f = Dot3(d2, r);
//
//		double s = 0.0f;
//		double t = 0.0f;
//
//		// 두 선분이 둘 다 점인 경우
//		if (a <= EPS && e <= EPS)
//			return Dot3(r, r);
//
//		// 첫 선분이 점인 경우
//		if (a <= EPS)
//		{
//			s = 0.0f;
//			t = (e > EPS) ? Clamp01(f / e) : 0.0f;
//		}
//		else
//		{
//			double c = Dot3(d1, r);
//
//			// 두 번째 선분이 점인 경우
//			if (e <= EPS)
//			{
//				t = 0.0f;
//				s = Clamp01(-c / a);
//			}
//			else
//			{
//				double b = Dot3(d1, d2);
//				double denom = a * e - b * b;
//
//				// 일반적인 경우: 내부 해
//				if (fabsf(denom) > EPS)
//					s = Clamp01((b * f - c * e) / denom);
//				else
//					s = 0.0f; // 거의 평행이면 임의로 s=0에서 시작
//
//				// s가 정해지면 t를 계산
//				t = (b * s + f) / e;
//
//				// t가 범위를 벗어나면 t를 고정하고 s를 재계산(여기가 핵심)
//				if (t < 0.0f)
//				{
//					t = 0.0f;
//					s = Clamp01(-c / a);
//				}
//				else if (t > 1.0f)
//				{
//					t = 1.0f;
//					s = Clamp01((b - c) / a);
//				}
//			}
//		}
//
//		XMVECTOR c1 = p1 + d1 * s;
//		XMVECTOR c2 = p2 + d2 * t;
//		XMVECTOR diff = c1 - c2;
//		return Dot3(diff, diff);
//	}
//
//	inline bool CheckCapsuleVsCapsule(const CapsuleView& c1, const CapsuleView& c2)
//	{
//		XMVECTOR c1A = XMLoadFloat3(&c1.p0);
//		XMVECTOR c1B = XMLoadFloat3(&c1.p1);
//		XMVECTOR c2A = XMLoadFloat3(&c2.p0);
//		XMVECTOR c2B = XMLoadFloat3(&c2.p1);
//
//		double distSq = SegmentSegmentDistSq(c1A, c1B, c2A, c2B);
//		double R = c1.radius + c2.radius;
//
//		return distSq <= R * R;
//	}
//}