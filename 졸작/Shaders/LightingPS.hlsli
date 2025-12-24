#include "Shadow.hlsli"
#include "InOutFormats.hlsli"
#include "PBR.hlsli"
#include "Fog.hlsli"

float4 PSMain(LIGHTING_PS_IN input) : SV_Target
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
            
            lightContribution = CalculatePBR(N, V, L, baseColor, metallic, roughness, radiance);
            
            if (i == 0)
            {
                float3 ambient = baseColor * 0.3 * ao; 
                lightContribution += ambient;
                
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
                float3 L = normalize(lightVec);
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation;
                
                float3 radiance = lights[i].color * lights[i].intensity * attenuation;
                
                lightContribution = CalculatePBR(N, V, L, baseColor, metallic, roughness, radiance);
            }
        }
        
        finalColor += lightContribution;
    }
    
    // Apply SSAO
    if (ssaoValue != 0.0f)
    {
        float ssaoStrength = 0.8;
        ssaoValue = lerp(1.0, ssaoValue, ssaoStrength);
        finalColor *= ssaoValue;
    }
        
    // Add emission
    finalColor += emission;
    
    finalColor = ApplyFog(finalColor, worldPos);
    
    return float4(finalColor, alpha);
}
