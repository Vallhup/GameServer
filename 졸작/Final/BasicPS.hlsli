
cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    float heightScale;
    float2 padding;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

Texture2D tex : register(t1);
SamplerState sampler1 : register(s1);

float4 PSMain(PS_IN input) : SV_Target
{
    if (useTexture)
        return tex.Sample(sampler1, input.uv);
    else
    {
        float4 color = float4(0.0f, 0.0f, 0.0f, 0.1f);
        return color;
    }
}