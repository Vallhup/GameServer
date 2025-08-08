
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
       // Base Color (Albedo)
        float4 baseColor = baseColorTex.Sample(sampler2, input.uv);
       
       // Normal Map
        float3 normalMap = normalTex.Sample(sampler2, input.uv).rgb;
       // Normal을 [-1, 1] 범위로 변환
        normalMap = (normalMap - 0.5) * 2.0;
       
       // Ambient Occlusion
        float ao = aoTex.Sample(sampler2, input.uv).r;
       
       // Roughness (Specular의 반대 개념)
        float roughness = roughnessTex.Sample(sampler2, input.uv).r;
        float specular = 1.0 - roughness;
       
       // 자연스러운 태양 조명 (오후 햇빛 각도)
        float3 lightDir = normalize(float3(0, 0, -1)); // 태양 각도
        float3 worldNormal = normalize(input.normal + normalMap * 1.0); // 노멀맵 적용
       
        float NdotL = max(0.0, dot(worldNormal, -lightDir));
       
       // 최종 색상 조합
        float3 diffuse = baseColor.rgb * NdotL;
        float3 ambient = baseColor.rgb * 0.2 * ao; // 약간 밝은 자연 환경광
        float3 finalColor = diffuse + ambient;
       
       // Specular 하이라이트 (태양 반사)
        float3 viewDir = normalize(float3(0.1, 0.1, -1)); // 약간 각도 있는 시선
        float3 reflectDir = reflect(lightDir, worldNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0) * specular * 0.8; // 태양 반사
        finalColor += spec;
       
        return float4(finalColor, baseColor.a);
    }
    else
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}