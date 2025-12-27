#ifndef INOUTFORMATS_HLSLI
#define INOUTFORMATS_HLSLI

//-------------------------------------------------------
// About checking depth and making shadow texture
//-------------------------------------------------------

struct SHADOW_VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

struct SHADOW_VS_OUT
{
    float4 pos : SV_POSITION;
};

struct SHADOW_PS_IN
{
    float4 pos : SV_POSITION;
};

//-------------------------------------------------------
// About forward rendering
//-------------------------------------------------------

struct FORWARD_VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

struct FORWARD_VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
    float3 worldPos : POSITION;
};

struct FORWARD_PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
    float3 worldPos : POSITION;
};

//-------------------------------------------------------
// About g-buffer and making four g-buffer render targets
//-------------------------------------------------------

struct GBUFFER_VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

struct GBUFFER_VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
    float4 worldPos : POSITION;
};

struct GBUFFER_PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
    float4 worldPos : POSITION;
};

struct GBUFFER_PS_OUT
{
    float4 RT0 : SV_Target0; // BaseColor.rgb + Metallic.r
    float4 RT1 : SV_Target1; // Normal.xyz + Roughness.r  
    float4 RT2 : SV_Target2; // WorldPos.xyz + AO.r
    float4 RT3 : SV_Target3; // Emission.rgb + Alpha.r (¶Ç´Â MaterialID)
};

//-------------------------------------------------------
// About deferred rendering
//-------------------------------------------------------

struct FULLSCREEN_VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct LIGHTING_PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

#endif