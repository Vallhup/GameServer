# Point Light 정적 그림자 (Cube Array Bake) — 작업 요약 (2026-06-11)

## 목표
FinalScene(성당)의 포인트 라이트 전체(~143개)에 대해 정적 객체 그림자를 표현.
라이트·정적 지오메트리가 모두 불변이므로 **씬 진입 시 1회 베이크 → 이후 런타임 비용 0** 구조.
(UE4 Stationary Light + Cached Shadow Map, UE5 VSM static page cache와 같은 사상)

## 설계
- **TextureCubeArray** (160 라이트 × 6면 = 960 slice, 256², D16) — 2D atlas 대신 큐브 배열을 써서
  HW 면 선택 + `SampleCmpLevelZero` HW PCF 사용. UV 변환 버퍼/타일 gutter 불필요
- **slice = lightIndex − 1** 고정 매핑 (lights[0]=태양, lights[1..N]=포인트) → 별도 매핑 버퍼 없음
- **베이크**: 라이트별로 OBB(정적 인스턴스) vs range 구체 충돌 검사로 캐스터 선별 → 6면 depth pass
- **동적 객체**(캐릭터/보스) 그림자는 기존 overhead 패스가 그대로 담당 → copy+overlay 자체가 불필요
- **샘플링**: 빛→픽셀 방향으로 큐브 면 선택, major-axis 거리를 베이크와 동일한 90° 투영식으로
  NDC depth 복원 후 비교. far = 라이트 range (라이트별로 다름, `lights[i].range` 재사용)
- VRAM: 960 × 256² × 2B ≈ **126MB**

## 변경 파일
### C++
| 파일 | 내용 |
|---|---|
| `ShadowMappingManager.h/.cpp` | 큐브 배열 리소스 + 면별 DSV(960) + 면별 VP CB 풀 + 베이크 인스턴스 풀(4MB). `WritePointShadowFaceCB`(D3D 큐브 면 규약 LookTo + PerspectiveFov 90°), PSR↔DW 전환 헬퍼, baked 플래그. `CascadeShadowConstants.shadowPad2` → `pointShadowCount / pointShadowStrength(0.85) / pointShadowNear` |
| `RootSignature.cpp` | **[4] b4 reserved(root CBV, 2 DWORD) → t14 SRV 테이블(1 DWORD)로 교체.** 기존 루트 시그니처가 정확히 64 DWORD 만석이라 신규 추가 불가 → 빈 슬롯 재활용으로 63 DWORD. 인덱스 안 밀림 |
| `Shader.h/.cpp` | `PSOType::PointShadow` — ShadowVS/PS 재사용, DSV D16 + DepthBias 32 / SlopeScaledDepthBias 1.5 |
| `RenderTargets.h/.cpp` | deferredSRVHeap 7→8, 슬롯 7에 TextureCubeArray SRV(R16_UNORM, NumCubes 160), `GetPointShadowSRV()` |
| `SceneRenderer.h/.cpp` | `RenderPointShadowChunk` — 베이크용 인스턴스드 드로우 (root 12에 풀 오프셋 직접 바인딩, PSO/루트 상태는 호출측 전제) |
| `DX12Core.h/.cpp` | `BakePointShadows`: 라이트별 캐스터 컬링(OBB vs BoundingSphere, OBB 없으면 거리 fallback) → 인스턴스 풀 순차 업로드 → 면별 clear+draw → PSR 전환 + CB 즉시 업로드. `Update()`에서 Final 이탈 시 count=0 + 재베이크 플래그 리셋. `LightingPass`에 테이블 4 바인딩 |
| `Engine.cpp` | Final 첫 프레임 `!IsPointShadowBaked()`면 베이크 (overhead 패스 직전) |
| `InstancingBatch.h` / `Scene.h` | `GetMesh()/IsCastShadow()` / `GetInstancingBatches()` 게터 |

### HLSL
| 파일 | 내용 |
|---|---|
| `ShaderResources.hlsli` | `TextureCubeArray pointShadowMaps : register(t14)` + b5에 상수 3개 |
| `Shadow.hlsli` | `SamplePointShadow` — normal offset 0.03, major-axis NDC 복원, bias 0.0005 |
| `LightingPS.hlsli` | 포인트 분기에서 `slice = i-1` 샘플, `lightContribution *= lerp(1-strength, 1, s)` |

## 트러블슈팅 이력
1. **루트 시그니처 64 DWORD 초과** — 기존이 정확히 64로 만석 (CBV 13×2 + SRV 8×2 + UAV 4×2 + 테이블 5 + 상수 9).
   → 미사용 b4 reserved 슬롯을 테이블로 교체해 해결 (위 표 참조)
2. **range 큰 라이트의 원거리 캐스터 그림자 소멸** — 90° 투영 NDC depth가 비선형이라 라이트에서
   멀수록 1.0 근처에 몰림. range 7m에서 캐스터 5m/수신면 6m의 NDC 차이가 ~0.0005인데 bias 0.002가
   통째로 삼킴. → near 0.05→**0.2** (정밀도 분포 개선) + bias 0.002→**0.0005**

## 동작 범위
- **Final 전용**: Engine 트리거가 SceneType::Final 게이트 + 타 씬은 pointShadowCount=0이라 셰이더도 비활성
- First/Second는 기존 그대로 (CSM + 포인트 라이트 조명만, 그림자 없음).
  확장하려면 Engine의 씬 타입 조건만 풀면 됨 (라이트 수 ≤ 160 확인)

## 튜닝 포인트
- 그림자 진하기: `pointShadowStrength` (csmConstants, 기본 0.85)
- acne 발생 시: 셰이더 bias 0.0005→0.001, 또는 PSO DepthBias(32)/slope(1.5)
- 촛대 등 라이트 20cm 이내 캐스터의 자기 그림자가 빠지면: `POINT_SHADOW_NEAR` 0.2→0.1~0.15 절충
- 특정 객체만 그림자가 없으면: 해당 배치 `castShadow` 플래그 확인 (false면 베이크 제외)

## 남은 확인
- [ ] 빌드/실기 검증 (특히 시연 PC 5060Ti — VRAM 126MB 추가)
- [ ] near 0.2 변경 후 근접 캐스터 그림자 상태
- [ ] 라이트 밀집 구역(제단 등) 프레임 확인
- [ ] (선택) pointShadowStrength ImGui 노출, PCF 멀티탭
