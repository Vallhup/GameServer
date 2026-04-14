#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"
#include "Constants.hlsli"

GBUFFER_PS_OUT PSMain(GBUFFER_PS_IN input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    GBUFFER_PS_OUT output;
    
    if (!isFrontFace)
    {
        input.normal = -input.normal;
        input.tangent = -input.tangent;
    }

    if (useTexture)
    {
        MaterialData material = materialBuffer[input.materialIndex];

        float4 baseColor = float4(1, 1, 1, 1);
        float3 normalMap = float3(0, 0, 1);
        float roughness = 0.5f;
        float metallic = 0.0f;
        float ao = 1.0f;
        float3 emission = float3(0, 0, 0);
        float height = 0.0f;
        
        if (useTerrainBlend)
        {
            float s = saturate(dot(normalize(input.normal), float3(0, 1, 0)));
            float t = smoothstep(SLOPE_THRESHOLD - SLOPE_SMOOTH, SLOPE_THRESHOLD + SLOPE_SMOOTH, s);
            
            float3 rockCol = bindlessTextures[NonUniformResourceIndex(ROCK_DIFFUSE_IDX)].Sample(linearSampler, input.uv * ROCK_TILING).rgb;
            float3 grassCol = bindlessTextures[NonUniformResourceIndex(GRASS_IDX)].Sample(linearSampler, input.uv * GRASS_TILING).rgb;
            
            baseColor = float4(lerp(rockCol, grassCol, t), 1.0);
            
            float3 rockNormal = bindlessTextures[NonUniformResourceIndex(ROCK_NORMAL_IDX)].Sample(linearSampler, input.uv * ROCK_TILING).rgb;
            rockNormal = (rockNormal - 0.5) * 2.0;
            normalMap = lerp(rockNormal, float3(0, 0, 1), t);
            
            roughness = lerp(0.85, 0.6, t);
        }
        else
        {
            if (material.baseColorTexIndex != 0xFFFFFFFF)
                baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(linearSampler, input.uv);
            
            float finalAlpha = baseColor.a;
            if (material.alphaTexIndex != 0xFFFFFFFF)
            {
                finalAlpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(linearSampler, input.uv).r;
                clip(finalAlpha - 0.5f);
            }
            else
            {
                clip(finalAlpha - 0.01f);
            }
            
            if (material.normalTexIndex != 0xFFFFFFFF)
            {
                normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(linearSampler, input.uv).rgb;
                normalMap = (normalMap - 0.5) * 2.0;
            }
        
            if (material.roughnessTexIndex != 0xFFFFFFFF)
                roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(linearSampler, input.uv).r;
                
            if (material.metallicTexIndex != 0xFFFFFFFF)
                metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(linearSampler, input.uv).r;
        
            if (material.emissionTexIndex != 0xFFFFFFFF)
                emission = bindlessTextures[NonUniformResourceIndex(material.emissionTexIndex)].Sample(linearSampler, input.uv).rgb;
        
            if (material.aoTexIndex != 0xFFFFFFFF)
                ao = bindlessTextures[NonUniformResourceIndex(material.aoTexIndex)].Sample(linearSampler, input.uv).r;
        
            if (material.heightTexIndex != 0xFFFFFFFF)
                height = bindlessTextures[NonUniformResourceIndex(material.heightTexIndex)].Sample(linearSampler, input.uv).r;
        }
        
        float3 worldNormal = ApplyNormalMap(input.normal, input.tangent, normalMap);
        
        output.RT0 = float4(baseColor.rgb, metallic);
        output.RT1 = float4(worldNormal, roughness);
        output.RT2 = float4(emission, ao);
    }
    else
    {
        output.RT0 = float4(input.color.rgb, 0);
        output.RT1 = float4(normalize(input.normal), 0.8);
        output.RT2 = float4(0, 0, 0, 1.0);
    }
    
    return output;
}
