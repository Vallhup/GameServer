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
    int useTerrainBlend;
    uint splatmap1Index;
    uint splatmap2Index;
    float splatUVScale;
    int splatLayerCount;
    float dissolveAmount;
    uint dissolveNoiseIndex;
    int objPadding;
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

struct LightData
{
    float3 position;
    float range;
    float3 color;
    float intensity;
    int type;
    float3 lightPadding;
};

cbuffer DeferredLightCB : register(b3)
{
    int lightCount;
    float3 deferredLightPadding;
};

// b4 reserved (legacy SunCB removed; sun is now lights[0] from t11)

cbuffer ShadowFrameCB : register(b5)
{
    matrix lightVP[3];
    float4 cascadeSplit;
    float shadowAmbientMin;
    float shadowFloor;
    float2 shadowPad;
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
    float4 ssaoSamples[32];
    float2 noiseScale;
    float samplingRadius;
    float ssaoBias;
};

cbuffer SkyboxCB : register(b9)
{
    float3 skyTintColor;
    float skyExposure;
    float skySaturation;
    uint skyIdx;
    uint skyIrrIdx;
    uint skyRadIdx;
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

cbuffer BloomCB : register(b13)
{
    uint bloomSrcMipIndex;
    float bloomFilterRadius;
    float2 bloomSrcTexelSize;
    float bloomIntensity;
    float bloomThreshold;
    float bloomKnee;
    uint bloomIsFirstPass;
};

cbuffer ClusterParamsCB : register(b14)
{
    uint3 clusterGridDims;
    float clusterZNear;
    float clusterZFar;
    float clusterSliceScale;
    float clusterSliceBias;
    float clusterPad0;
};

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

StructuredBuffer<LightData> lights : register(t11);

// Clustered Shading — PS read view
StructuredBuffer<uint>  clusterLightIndices : register(t12);
StructuredBuffer<uint2> clusterLightGrid    : register(t13);

// Clustered Shading — CS write view (same resources as t12/t13/counter)
RWStructuredBuffer<uint>  clusterLightIndicesRW : register(u1);
RWStructuredBuffer<uint2> clusterLightGridRW    : register(u2);
RWStructuredBuffer<uint>  clusterCounterRW      : register(u3);

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
SamplerState linearClampSampler : register(s2);
SamplerComparisonState shadowCmpSampler : register(s3);

#endif