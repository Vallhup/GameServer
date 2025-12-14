cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
    float3 cameraPosition;
    float framePadding;
};

cbuffer LightCB : register(b3)
{
    int lightCount;
    float3 lightPadding;
    
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

// PBR Constants
static const float PI = 3.14159265359;

// Fresnel-Schlick approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// GGX/Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / max(denom, 0.0001);
}

// Smith's method with Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / max(denom, 0.0001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Cook-Torrance BRDF
float3 CalculatePBR(float3 N, float3 V, float3 L, float3 baseColor,
                    float metallic, float roughness, float3 radiance)
{
    float3 H = normalize(V + L);
    
    // Calculate F0 (surface reflection at zero incidence)
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, baseColor, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * baseColor / PI + specular) * radiance * NdotL;
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
    return lerp(1.0, 0.8, shadow);
}

float4 PSMain(PS_IN input) : SV_Target
{
    float4 rt0 = gBufferRT0.Sample(pointSampler, input.uv);
    float4 rt1 = gBufferRT1.Sample(pointSampler, input.uv);
    float4 rt2 = gBufferRT2.Sample(pointSampler, input.uv);
    float4 rt3 = gBufferRT3.Sample(pointSampler, input.uv);
    
    if (rt0.a == 0.0f && rt1.a == 0.0f && rt2.a == 0.0f && rt3.a == 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    float3 baseColor = rt0.rgb;
    float metallic = rt0.a;
    
    float3 worldNormal = normalize(rt1.xyz);
    float roughness = rt1.w;
    
    float3 worldPos = rt2.xyz;
    float ao = rt2.a;
    
    float3 emission = rt3.rgb;
    float alpha = rt3.a;
    
    float3 N = worldNormal;
    float3 V = normalize(cameraPosition - worldPos);
    
    float3 finalColor = float3(0, 0, 0);
    float ssaoValue = ssaoMap.Sample(pointSampler, input.uv).r;
    
    for (int i = 0; i < lightCount; ++i)
    {
        float3 lightContribution = float3(0, 0, 0);
        
        if (lights[i].type == 0) // Directional Light
        {
            float3 L = normalize(-lights[i].position);
            float3 radiance = lights[i].color * lights[i].intensity;
            
            // PBR lighting
            lightContribution = CalculatePBR(N, V, L, baseColor, metallic, roughness, radiance);
            
            // Ambient 추가 (첫 번째 라이트에만)
            if (i == 0)
            {
                float3 ambient = baseColor * 0.3 * ao; // ← 여기로 이동!
                lightContribution += ambient;
                
                float shadow = CalculateShadow(worldPos);
                lightContribution *= shadow; // ← ambient도 그림자 적용!
            }
        }
        else if (lights[i].type == 1) // Point Light
        {
            float3 lightVec = lights[i].position - worldPos;
            float distance = length(lightVec);
            
            if (distance < lights[i].range)
            {
                float3 L = normalize(lightVec);
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation;
                
                float3 radiance = lights[i].color * lights[i].intensity * attenuation;
                
                // PBR lighting
                lightContribution = CalculatePBR(N, V, L, baseColor, metallic, roughness, radiance);
            }
        }
        
        finalColor += lightContribution;
    }
    
    // Ambient 제거 (위로 이동했으므로)
    // float3 ambient = baseColor * 0.3 * ao;
    // finalColor += ambient;
    
    // Apply SSAO
    if (ssaoValue != 0.0f)
    {
        float ssaoStrength = 0.8;
        ssaoValue = lerp(1.0, ssaoValue, ssaoStrength);
        finalColor *= ssaoValue;
    }
        
    // Add emission
    finalColor += emission;
    
    return float4(finalColor, alpha);
}
