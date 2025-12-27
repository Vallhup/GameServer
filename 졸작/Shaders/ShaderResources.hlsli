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
    float3 cameraPosition;
    float framePadding;
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
    } lights[25];
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
    matrix lightView;
    matrix lightProjection;
};

cbuffer SSAOConstants : register(b6)
{
    float4 ssaoOffsetVectors[14];
    matrix ssaoProjection;
    float occlusionRadius;
    float occlusionFadeStart;
    float occlusionFadeEnd;
    float surfaceEpsilon;
    float3 ssaoPadding;
};

cbuffer FogConstants : register(b7)
{
    float4 fogColor;
    float fogStart;     // 거리 안개 시작
    float fogRange;     // 거리 안개 범위
    float fogZoneStart; // Z축 안개 시작점
    float fogZoneEnd;   // Z축 안개 끝점
    float fogZoneFade;  // 보간 거리
    float3 fogPadding;
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
Texture2D gBufferRT2 : register(t6); // WorldPos + AO
Texture2D gBufferRT3 : register(t7); // Emission + Alpha
Texture2D shadowMap : register(t8);
Texture2D ssaoMap : register(t9);

//-------------------------------------------------------
// SSAO PARAMETERS NOT ADDED YET
//-------------------------------------------------------


//-------------------------------------------------------
// SAMPLERS
//-------------------------------------------------------

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

#endif