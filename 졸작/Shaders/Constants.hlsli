#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

//-------------------------------------------------------
// Common Constants
//-------------------------------------------------------

static const float PI = 3.14159265359;

//-------------------------------------------------------
// CSM Constants
//-------------------------------------------------------

const static float cascadeBias[4] = { 0.0001f, 0.0005f, 0.0000f, 0.0000f };

//-------------------------------------------------------
// Volumetric Fog Constants (Test)
//-------------------------------------------------------

static const float VF_DENSITY = 0.02f;                               // 안개 밀도
static const float VF_SCATTERING = 0.8f;                             // 산란 계수
static const float VF_ABSORPTION = 0.1f;                             // 흡수 계수
static const float VF_HG_ANISOTROPY = 0.6f;                          // Phase function g값 (0=등방, 1=전방산란)

static const int VF_MAX_STEPS = 16;                                  // Ray March 스텝 수
static const float VF_MAX_DISTANCE = 160.0f;                         // 최대 거리
static const float VF_JITTER_STRENGTH = 0.5f;                        // Banding 완화용

static const float VF_HEIGHT_FALLOFF = 0.001f;                       // 높이 감쇠 계수
static const float VF_GROUND_HEIGHT = 3.0f;                          // 기준 높이

static const float3 VF_LIGHT_COLOR = float3(1.0f, 1.0f, 1.0f);       // 안개 속 빛 색상
static const float VF_LIGHT_INTENSITY = 1.5f;                        // 빛 강도

//-------------------------------------------------------
// Ssao Constants
//-------------------------------------------------------

static const int KERNEL_SIZE = 16;

#endif