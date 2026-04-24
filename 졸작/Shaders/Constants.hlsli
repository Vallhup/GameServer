#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

//-------------------------------------------------------
// Common Constants
//-------------------------------------------------------

static const float PI = 3.14159265359;
#define HDR_SCENE_BINDLESS_INDEX 9000
#define BLOOM_MIP_BASE           9001

//-------------------------------------------------------
// CSM Constants
//-------------------------------------------------------

static const float cascadeBias[4] = { 0.0002f, 0.0005f, 0.0012f, 0.0000f };
static const float cascadeNormalOffset[4] = { 0.015f, 0.05f, 0.15f, 0.0f };
static const float CASCADE_BLEND_RANGE = 0.15;

//-------------------------------------------------------
// Volumetric Fog Constants (cbuffer에서 참조)
//-------------------------------------------------------

#define VF_DENSITY          vfDensity
#define VF_SCATTERING       vfScattering
#define VF_ABSORPTION       vfAbsorption
#define VF_HG_ANISOTROPY    vfHgAnisotropy
#define VF_MAX_STEPS        vfMaxSteps
#define VF_MAX_DISTANCE     vfMaxDistance
#define VF_JITTER_STRENGTH  vfJitterStrength
#define VF_HEIGHT_FALLOFF   vfHeightFalloff
#define VF_GROUND_HEIGHT    vfGroundHeight
#define VF_LIGHT_COLOR      vfLightColor
#define VF_LIGHT_INTENSITY  vfLightIntensity

//-------------------------------------------------------
// Ssao Constants
//-------------------------------------------------------

static const int KERNEL_SIZE = 32;

//-------------------------------------------------------
// IBL
//-------------------------------------------------------

static const uint BRDF_LUT_INDEX = 0;

//-------------------------------------------------------
// Mountain Texture tiling Constants
//-------------------------------------------------------

static const int ROCK_DIFFUSE_IDX = 1;
static const int ROCK_NORMAL_IDX = 2;
static const int GRASS_IDX = 3;
static const int GRASS_VARIANT_IDX = 4;

static const float2 ROCK_TILING = float2(10.0, 12.56);
static const float GRASS_TILING = 150.0;
static const float SLOPE_THRESHOLD = 0.65;
static const float SLOPE_SMOOTH = 0.1;

#endif