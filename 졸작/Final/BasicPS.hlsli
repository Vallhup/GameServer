
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
Texture2D baseColorTex : register(t2);
Texture2D normalTex : register(t3);
Texture2D roughnessTex : register(t4);
Texture2D metallicTex : register(t5);
Texture2D heightTex : register(t6);
Texture2D alphaTex : register(t7);
Texture2D emissionTex : register(t8);
Texture2D aoTex : register(t9);

SamplerState sampler1 : register(s1);
SamplerState sampler2 : register(s2);

float4 PSMain(PS_IN input) : SV_Target
{
    if (useTexture)
    {
        return float4(input.uv, 0.0, 1.0); // UV를 색상으로 시각화
    }
    else
    {
        float4 color = float4(1.0f, 0.0f, 0.0f, 1.0f);
        return color;
    }
}