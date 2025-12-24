#ifndef CONSTANT_BUFFERS_HLSLI
#define CONSTANT_BUFFERS_HLSLI

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
    
    float padding;
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

#endif