#ifndef PBR_HLSLI
#define PBR_HLSLI
#include "Constants.hlsli"

// Fresnel-Schlick approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel-Schlick with roughness
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
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

float3 CalculateIBL(float3 N, float3 V, float3 baseColor, float metallic,
    float roughness, float ao, TextureCube irradianceMap, TextureCube radianceMap,
    Texture2D brdfLUT, SamplerState samp)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, baseColor, metallic);
    
    float NdotV = max(dot(N, V), 0.0);
    
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    float3 irradiance = irradianceMap.Sample(samp, float3(N.x, -N.y, N.z)).rgb;
    float3 diffuseIBL = irradiance * baseColor;
    
    float3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 7.0;
    float3 prefilteredColor = radianceMap.SampleLevel(samp, float3(R.x, -R.y, R.z), roughness * MAX_REFLECTION_LOD).rgb;
    
    float2 brdfUV = float2(NdotV, roughness);
    float2 brdf = brdfLUT.Sample(samp, brdfUV).rg;
    
    float3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);
    
    float diffuseIntensity = 1.0f; 
    float specularIntensity = 1.0f;
    
    float3 ambient = (kD * diffuseIBL * diffuseIntensity + specularIBL * specularIntensity) * ao;
    
    return ambient;
}

#endif

