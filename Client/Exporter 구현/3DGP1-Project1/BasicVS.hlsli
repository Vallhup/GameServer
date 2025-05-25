struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix proj;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : UV;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = float4(input.pos, 1.0f);
    output.pos = mul(worldPos, view);
    output.pos = mul(output.pos, proj);
    output.uv = input.uv;
    return output;
}
