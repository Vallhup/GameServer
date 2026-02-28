#ifndef VOLUMETRICFOG_HLSLI
#define VOLUMETRICFOG_HLSLI

#include "ShaderResources.hlsli"
#include "Constants.hlsli"

float GetFogDensity(float3 worldPos)
{
    float density = exp(-VF_HEIGHT_FALLOFF * max(0.0, worldPos.y - VF_GROUND_HEIGHT));
    return VF_DENSITY * density;
}

float BeerLambert(float density, float distance)
{
    float sigma_t = density * (VF_SCATTERING + VF_ABSORPTION);
    return exp(-sigma_t * distance);
}

float SampleShadowMap(float3 worldPos, float viewDepth)
{
    int cascade = 3;
    if (viewDepth < cascadeSplit.x)
        cascade = 0;
    if (viewDepth < cascadeSplit.y)
        cascade = 1;
    if (viewDepth < cascadeSplit.z)
        cascade = 2;
    
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightVP[cascade]);
    
    lightSpacePos.xyz /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;
    
    float currentDepth = lightSpacePos.z;
    float shadowMapDepth = shadowMapArray.Sample(linearSampler, float3(shadowUV, cascade)).r;
    
    float bias = 0.0001f;
    return (currentDepth - bias) > shadowMapDepth ? 0.0 : 1.0;
}

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float expression = 1 + g2 - 2.0 * g * cosTheta;
    return (1 - g2) / (4.0 * PI * max(expression, 0.0001) * sqrt(max(expression, 0.0001)));
}

float GetJitter(float2 uv)
{
    float2 seed = uv * 1000.0;
    return frac(sin(dot(seed, float2(12.9898, 78.233))) * 43758.5453) * VF_JITTER_STRENGTH;
}

float4 RayMarchingVolumetricFog(float3 rayOrigin, float3 rayDir, float sceneDepth, float2 screenUV)
{
    float marchDistance = min(sceneDepth, VF_MAX_DISTANCE);
    float stepSize = marchDistance / (float)VF_MAX_STEPS;
    
    float jitter = GetJitter(screenUV);
    float currentDistance = jitter * stepSize;
    
    // 
    // 누적 변수
    float3 totalInScattering = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;

     // 주 광원 방향 (lights[0]가 Directional Light)
    float3 lightDir = normalize(-lights[0].position);

     // Ray Marching Loop
    [loop]
    for (int i = 0; i < VF_MAX_STEPS; ++i)
    {
         // Early exit
        if (transmittance < 0.01 || currentDistance >= marchDistance)
            break;

        float3 samplePos = rayOrigin + rayDir * currentDistance;

         // 현재 위치의 밀도
        float density = GetFogDensity(samplePos);

        if (density > 0.0001)
        {
             // Shadow 체크 (Light Shaft 효과)
            float shadowFactor = SampleShadowMap(samplePos, currentDistance);

             // Phase function
            float cosTheta = dot(rayDir, lightDir);
            float phase = HenyeyGreenstein(cosTheta, VF_HG_ANISOTROPY);
            phase = max(phase, 0.2);
            
             // In-scattering 계산
            float3 lightContrib = VF_LIGHT_COLOR * VF_LIGHT_INTENSITY * lights[0].intensity;
            float3 scattering = lightContrib * phase * VF_SCATTERING * shadowFactor;

             // Beer-Lambert 투과율
            float stepTransmittance = BeerLambert(density, stepSize);

             // 에너지 보존 적분
            float3 integScatter = scattering * (1.0 - stepTransmittance);
            totalInScattering += transmittance * integScatter;

            transmittance *= stepTransmittance;
        }

        currentDistance += stepSize;
    }

     // Ambient 산란 (fogColor 사용)
    float3 ambientScatter = fogColor.rgb * (1.0 - transmittance) * 0.3;
    totalInScattering += ambientScatter;

    return float4(totalInScattering, transmittance);
}

float3 ApplyVolumetricFog(float3 sceneColor, float3 worldPos, float2 screenUV, float3 camPos)
{
    float3 rayOrigin = camPos;
    float3 rayDir = normalize(worldPos - camPos);
    float sceneDepth = length(worldPos - camPos);
    
    float4 fogResult = RayMarchingVolumetricFog(rayOrigin, rayDir, sceneDepth, screenUV);
    
    // L = L_background x T + L_inscattering
    return sceneColor * fogResult.a + fogResult.rgb;
}

#endif