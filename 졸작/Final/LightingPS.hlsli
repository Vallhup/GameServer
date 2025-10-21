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
    } lights[25];
}

cbuffer shadowFrameCB : register(b5)
{
    matrix lightView;
    matrix lightProjection;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gBufferRT0 : register(t4); // BaseColor + Metallic
Texture2D gBufferRT1 : register(t5); // Normal + Roughness
Texture2D gBufferRT2 : register(t6); // WorldPos + AO
Texture2D gBufferRT3 : register(t7); // Emission + Alpha
Texture2D shadowMap : register(t8);
Texture2D ssaoMap : register(t9);

SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

float CalculateShadow(float3 worldPos)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightView);
    lightSpacePos = mul(lightSpacePos, lightProjection);
    
    lightSpacePos.xyz /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y; 
    
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0; 
    
    float currentDepth = lightSpacePos.z;
    float shadowMapDepth = shadowMap.Sample(linearSampler, shadowUV).r;
    
    float bias = 0.0001f;
    float shadow = 0.0f;
    float2 texelSize = 1.0 / 2048.0;
    
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float shadowMapDepth = shadowMap.Sample(linearSampler, shadowUV + offset).r;
            
            if ((currentDepth - bias) > shadowMapDepth)
                shadow += 1.0f;
        }
    }

    shadow /= 25.0;
    return lerp(1.0, 0.2, shadow);
}

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
    
    for (int i = 0; i < lightCount; ++i)
    {
        float3 lightContribution = float3(0, 0, 0);
        
        if (lights[i].type == 0) // Directional Light
        {
            float3 lightDir = normalize(-lights[i].position);
            float NdotL = max(0.0, dot(worldNormal, -lightDir));
            
            float3 diffuse = baseColor * NdotL * 0.7;
            float3 ambient = baseColor * 0.3; 
            
            float3 viewDir = normalize(float3(0.1, 0.1, -1)); 
            float3 reflectDir = reflect(lightDir, worldNormal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
            
            lightContribution = (diffuse + ambient + spec) * lights[i].color * lights[i].intensity;
            
            if (i == 0)
            {
                float shadow = CalculateShadow(worldPos);
                lightContribution *= shadow;
            }
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
                
                float3 diffuse = baseColor * NdotL * 0.7;
                float3 ambient = baseColor * 0.3;
                
                float3 viewDir = normalize(float3(0.1, 0.1, -1));
                float3 reflectDir = reflect(-lightDir, worldNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
                
                lightContribution = (diffuse + ambient + spec) * lights[i].color * lights[i].intensity * attenuation;
            }
        }
        
        finalColor += lightContribution;
    }
    
    finalColor *= ao;
    
    finalColor += emission;
    
    return float4(finalColor, alpha);
    
    // ShadowMap만 그릴때
    //float shadowDepth = shadowMap.Sample(pointSampler, input.uv).r;
    //return float4(shadowDepth, shadowDepth, shadowDepth, 1.0);
}