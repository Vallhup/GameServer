cbuffer LightCB : register(b3)
{
    int lightCount;
    float3 padding;
    
    struct LightData
    {
        float3 position;
        float range;
        float3 color;
        float intensity;
        int type;
        float3 lightPadding;
    } lights[50];
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
    // G-Buffer에서 데이터 샘플링
    float4 position = gBufferPosition.Sample(pointSampler, input.uv);
    float4 normal = gBufferNormal.Sample(pointSampler, input.uv);
    float4 albedo = gBufferAlbedo.Sample(pointSampler, input.uv);
    
    // 배경인 경우 (depth가 1.0)
    if (position.w == 0.0f)
    {
        return albedo;
    }
    
    float3 worldPos = position.xyz;
    float3 worldNormal = normalize(normal.xyz);
    float3 finalColor = float3(0, 0, 0);
    
    // 모든 조명에 대해 계산
    for (int i = 0; i < lightCount; ++i)
    {
        float3 lightContribution = float3(0, 0, 0);
        
        if (lights[i].type == 0) // Directional Light
        {
            float3 lightDir = normalize(-lights[i].position); // direction으로 사용
            float NdotL = max(0.0, dot(worldNormal, lightDir));
            lightContribution = albedo.rgb * lights[i].color * lights[i].intensity * NdotL;
        }
        else if (lights[i].type == 1) // Point Light
        {
            float3 lightVec = lights[i].position - worldPos;
            float distance = length(lightVec);
            
            // 범위 체크
            if (distance < lights[i].range)
            {
                float3 lightDir = normalize(lightVec);
                float NdotL = max(0.0, dot(worldNormal, lightDir));
                
                // 거리 감쇠
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation; // 제곱 감쇠
                
                lightContribution = albedo.rgb * lights[i].color * lights[i].intensity * NdotL * attenuation;
            }
        }
        
        finalColor += lightContribution;
    }
    
    // 약간의 ambient 추가
    finalColor += albedo.rgb * 0.1f;
    
    return float4(finalColor, albedo.a);
}