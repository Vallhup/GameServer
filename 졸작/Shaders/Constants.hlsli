#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

//-------------------------------------------------------
// Common Constants
//-------------------------------------------------------

static const float PI = 3.14159265359;

//-------------------------------------------------------
// CSM Constants
//-------------------------------------------------------

const static float cascadeBias[4] = { 0.0002f, 0.0006f, 0.0000f, 0.0000f };

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

#endif