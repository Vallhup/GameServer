
cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    float heightScale;
    int useInstancing;
    float padding;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
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
        float4 baseColor = baseColorTex.Sample(sampler2, input.uv);
        float3 normalMap = normalTex.Sample(sampler2, input.uv).rgb;
        normalMap = (normalMap - 0.5) * 2.0;
        float roughness = roughnessTex.Sample(sampler2, input.uv).r;
        float metallic = metallicTex.Sample(sampler2, input.uv).r;
        
        // 간단한 라이팅
        float3 lightDir = normalize(float3(0, 0, 1));
        float3 worldNormal = normalize(input.normal + normalMap * 0.3);
        float NdotL = max(0.0, dot(worldNormal, -lightDir));
        
        // 밝게 조정
        float3 diffuse = baseColor.rgb * NdotL * 0.7; // diffuse 줄임
        float3 ambient = baseColor.rgb * 0.6; // ambient 크게 늘림
        
        // Specular는 금속에서만
        float3 viewDir = normalize(float3(0.1, 0.1, -1));
        float3 reflectDir = reflect(lightDir, worldNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
        
        float3 finalColor = diffuse + ambient + spec;
        
        return float4(finalColor, baseColor.a);
    }
    else
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}