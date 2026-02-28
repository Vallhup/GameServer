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
};

cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    int useInstancing;
    uint materialIndex;
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
    matrix lightVP[4];
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

Texture2D gBufferRT0 : register(t4); // BaseColor + Metallic
Texture2D gBufferRT1 : register(t5); // Normal + Roughness
Texture2D gBufferRT2 : register(t6); // Emission + AO
Texture2D depthBuffer : register(t7); // Depth
Texture2DArray shadowMapArray : register(t8);

TextureCube bindlessCubeMaps[] : register(t0, space3);

//-------------------------------------------------------
// SAMPLERS
//-------------------------------------------------------

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

//-------------------------------------------------------
// INDEXES
//-------------------------------------------------------

static const uint IBL_IRRADIANCE_INDEX = 1;
static const uint IBL_RADIANCE_INDEX = 2;
static const uint BRDF_LUT_INDEX = 120;

#endif