
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
    float2 padding;
};

Texture2D heightmapTexture : register(t0);
SamplerState heightmapSampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    
    float3 modifiedPos = input.pos;
    
    if (heightScale > 0.0f)
    {
        float height = heightmapTexture.SampleLevel(heightmapSampler, input.uv, 0).r;
        modifiedPos.y += height * heightScale;
    }
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}