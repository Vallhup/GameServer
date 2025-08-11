
cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
};

cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    float heightScale;
    int useInstancing;
    float padding;
};

Texture2D heightmapTexture : register(t0);
StructuredBuffer<matrix> instanceTransforms : register(t0, space1);
SamplerState heightmapSampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
};

VS_OUT VSMain(VS_IN input, uint instanceID : SV_InstanceID)
{
    VS_OUT output;
    
    float3 modifiedPos = input.pos;
    
    if (heightScale > 0.0f)
    {
        float height = heightmapTexture.SampleLevel(heightmapSampler, input.uv, 0).r;
        modifiedPos.y += height * heightScale;
    }
    
    matrix worldMatrix;
    if (useInstancing)
        worldMatrix = instanceTransforms[instanceID];
    else
        worldMatrix = world;
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    
    output.color = input.color;
    output.uv = input.uv;
    output.normal = input.normal; // ← 추가!
    output.tangent = input.tangent; // ← 추가!
    output.weights = input.weights; // ← 추가!
    output.indices = input.indices; // ← 추가!
    
    return output;
}