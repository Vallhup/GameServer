cbuffer LightCB : register(b3)
{
    float3 lightDirection;
    float padding;
    float3 lightColor;
    float lightIntensity;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gBufferPosition : register(t4); // G-Buffer RT0
Texture2D gBufferNormal : register(t5); // G-Buffer RT1  
Texture2D gBufferAlbedo : register(t6); // G-Buffer RT2
SamplerState pointSampler : register(s0);

float4 PSMain(PS_IN input) : SV_Target
{
    // G-Buffer에서 데이터 읽기
    float3 worldPos = gBufferPosition.Sample(pointSampler, input.uv).xyz;
    float3 worldNormal = normalize(gBufferNormal.Sample(pointSampler, input.uv).xyz);
    float3 albedo = gBufferAlbedo.Sample(pointSampler, input.uv).rgb;
    
    // 간단한 Directional Light 계산 (기존 Forward와 동일)
    float3 lightDir = normalize(-lightDirection);
    float NdotL = max(0.0f, dot(worldNormal, lightDir));
    
    // 라이팅 계산
    float3 diffuse = albedo * NdotL * 0.7f;
    float3 ambient = albedo * 0.3f; // 간단한 앰비언트
    
    float3 finalColor = (diffuse + ambient) * lightColor * lightIntensity;
    
    return float4(finalColor, 1.0f);
}