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
    } lights[30];
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gBufferRT0 : register(t4); // BaseColor + Metallic
Texture2D gBufferRT1 : register(t5); // Normal + Roughness
Texture2D gBufferRT2 : register(t6); // WorldPos + AO
Texture2D gBufferRT3 : register(t7); // Emission + Alpha
SamplerState pointSampler : register(s0);

float4 PSMain(PS_IN input) : SV_Target
{
    float4 rt0 = gBufferRT0.Sample(pointSampler, input.uv); // BaseColor + Metallic
    float4 rt1 = gBufferRT1.Sample(pointSampler, input.uv); // Normal + Roughness  
    float4 rt2 = gBufferRT2.Sample(pointSampler, input.uv); // WorldPos + AO
    float4 rt3 = gBufferRT3.Sample(pointSampler, input.uv); // Emission + Alpha
    
    if (rt0.a == 0.0f && rt1.a == 0.0f && rt2.a == 0.0f && rt3.a == 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f); // 기본 배경색
    }
    
    float3 baseColor = rt0.rgb;
    float metallic = rt0.a;
    
    float3 worldNormal = normalize(rt1.xyz);
    float roughness = rt1.w;
    
    float3 worldPos = rt2.xyz;
    float ao = rt2.a;
    
    float3 emission = rt3.rgb;
    float alpha = rt3.a;
    
    float3 finalColor = float3(0, 0, 0);
    
    // 모든 라이트에 대해 계산
    for (int i = 0; i < lightCount; ++i)
    {
        float3 lightContribution = float3(0, 0, 0);
        
        if (lights[i].type == 0) // Directional Light
        {
            float3 lightDir = normalize(-lights[i].position);
            float NdotL = max(0.0, dot(worldNormal, -lightDir));
            
            // BasicPS 스타일: Diffuse + Ambient + Specular
            float3 diffuse = baseColor * NdotL * 0.7;
            float3 ambient = baseColor * 0.3; // BasicPS에서는 1.0이었지만 너무 밝음
            
            // Specular 계산 (BasicPS 스타일)
            float3 viewDir = normalize(float3(0.1, 0.1, -1)); // BasicPS와 동일한 고정 viewDir
            float3 reflectDir = reflect(lightDir, worldNormal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
            
            lightContribution = (diffuse + ambient + spec) * lights[i].color * lights[i].intensity;
        }
        else if (lights[i].type == 1) // Point Light
        {
            float3 lightVec = lights[i].position - worldPos;
            float distance = length(lightVec);
            
            if (distance < lights[i].range)
            {
                float3 lightDir = normalize(lightVec);
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation;
                
                float NdotL = max(0.0, dot(worldNormal, lightDir));
                
                // BasicPS 스타일: Diffuse + Ambient + Specular
                float3 diffuse = baseColor * NdotL * 0.7;
                float3 ambient = baseColor * 0.3;
                
                // Specular 계산
                float3 viewDir = normalize(float3(0.1, 0.1, -1));
                float3 reflectDir = reflect(-lightDir, worldNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
                
                lightContribution = (diffuse + ambient + spec) * lights[i].color * lights[i].intensity * attenuation;
            }
        }
        
        finalColor += lightContribution;
    }
    
    // AO 적용 (전체 결과에 곱하기)
    finalColor *= ao;
    
    // Emission 추가
    finalColor += emission;
    
    return float4(finalColor, alpha);
}