#ifndef SHADERRESOURCES_HLSLI
#define SHADERRESOURCES_HLSLI

#include "CommonData.hlsli"

//-------------------------------------------------------
// CBV START
//-------------------------------------------------------

cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
    matrix invViewProj;
    float3 cameraPosition;
    float time;
    uint lutIndex;
    uint prevLutIndex;
    float lutBlendFactor;
    float saturationFactor;
};

cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    int useInstancing;
    uint materialIndex;
    int useVertexAnim;
};

cbuffer AnimationParams : register(b2)
{
    int aBoneCount;
    int aCurrentFrame;
    int aNextFrame;
    float aRatio;
    int aAnimationOffset;
    
    int isBlending;
    int aPrevCurrentFrame;
    int aPrevNextFrame;
    float aPrevRatio;
    int aPrevAnimationOffset;
    float aBlendRatio;
    
    float animationPadding;
};

cbuffer DeferredLightCB : register(b3)
{
    int lightCount;
    float3 deferredLightPadding;
    
    struct LightData
    {
        float3 position;
        float range;
        float3 color;
        float intensity;
        int type;
        float3 lightPadding;
    } lights[23];
};

cbuffer ForwardLightCB : register(b4)
{
    float3 lightDirection;
    float forwardLightPadding;
    float3 lightColor;
    float lightIntensity;
};

cbuffer ShadowFrameCB : register(b5)
{
    matrix lightVP[2];
    float4 cascadeSplit;
};

cbuffer FogConstants : register(b6)
{
    float4 fogColor;
    float fogStart;     
    float fogRange;     
    float fogZoneStart; 
    float fogZoneEnd;   
    float fogZoneFade;  
    float3 fogPadding;
};

cbuffer CascadeShadowIndex : register(b7)
{
    int cascadeIndex;
    int3 cascadePadding;
};

cbuffer SsaoCB : register(b8)
{
    float4 ssaoSamples[16];
    float2 noiseScale;
    float samplingRadius;
    float ssaoBias;
};

cbuffer SkyboxCB : register(b9)
{
    float3 skyTintColor;
    float skyExposure;
    float skySaturation;
    float3 skyPadding;
};

cbuffer WaterCB : register(b10)
{
    float4 waterColor;
    float waterTime;
    float waveSpeed;
    float waveStrength;
    float waterPadding;
};

cbuffer VolumetricFogCB : register(b11)
{
    float vfDensity;
    float vfScattering;
    float vfAbsorption;
    float vfHgAnisotropy;

    int vfMaxSteps;
    float vfMaxDistance;
    float vfJitterStrength;
    float vfHeightFalloff;

    float vfGroundHeight;
    float3 vfLightColor;

    float vfLightIntensity;
    float2 vfTexelSize;
    float vfPadding;
};

cbuffer EffectCB : register(b12)
{
    float4 effectColor;
    uint textureIndex;
    float3 padding;
}

//-------------------------------------------------------
// VARIOUS TYPES OF SHADER RESOURCES
//-------------------------------------------------------

Texture2D bindlessTextures[] : register(t0, space1);

StructuredBuffer<MaterialData> materialBuffer : register(t0);
StructuredBuffer<AnimFrameParams> aBoneFrame : register(t1);
StructuredBuffer<matrix> aOffset : register(t2);
StructuredBuffer<matrix> finalBoneTransforms : register(t3);

RWStructuredBuffer<matrix> aFinal : register(u0);

StructuredBuffer<matrix> instanceTransforms : register(t0, space2);

Texture2D gBufferRT0 : register(t4);    // BaseColor + Metallic
Texture2D gBufferRT1 : register(t5);    // Normal + Roughness
Texture2D gBufferRT2 : register(t6);    // Emission + AO
Texture2D depthBuffer : register(t7);   // Depth
Texture2DArray shadowMapArray : register(t8);
Texture2D ssaoTexture : register(t9);
Texture2D fogTexture : register(t10);

TextureCube bindlessCubeMaps[] : register(t0, space3);

Texture2D ssaoNormal : register(t0, space4);
Texture2D ssaoDepth : register(t1, space4);
Texture2D ssaoNoise : register(t2, space4);
Texture2D ssaoResult : register(t3, space4);

Texture3D bindlessTextures3D[] : register(t0, space5);

//-------------------------------------------------------
// SAMPLERS
//-------------------------------------------------------

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);
SamplerState lutLinearSampler : register(s2);

//-------------------------------------------------------
// INDEXES
//-------------------------------------------------------

static const uint IBL_IRRADIANCE_INDEX = 1;
static const uint IBL_RADIANCE_INDEX = 2;
static const uint BRDF_LUT_INDEX = 109;

#endif