#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"
#include "PBR.hlsli"
#include "Fog.hlsli"
#include "VolumetricFog.hlsli"
#include "ToneMapping.hlsli"

float4 PSMain(FORWARD_PS_IN input) : SV_Target
{
    if (useTexture)
    {
        MaterialData material = materialBuffer[input.materialIndex];
        
        float4 baseColor = float4(1, 1, 1, 1);
        float3 normalMap = float3(0, 0, 1);
        float roughness = 0.5f;
        float metallic = 0.0f;
        float alpha = 1.0f;
        
        if (material.baseColorTexIndex != 0xFFFFFFFF)
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(linearSampler, input.uv);
        
        if (material.alphaTexIndex != 0xFFFFFFFF)
            alpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(linearSampler, input.uv).r;
        
        float finalAlpha = baseColor.a * alpha;
        clip(finalAlpha - 0.01f);
        
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(linearSampler, input.uv).rgb;
            normalMap = (normalMap - 0.5) * 2.0;
        }
        
        if (material.roughnessTexIndex != 0xFFFFFFFF)
            roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(linearSampler, input.uv).r;
        
        if (material.metallicTexIndex != 0xFFFFFFFF)
            metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(linearSampler, input.uv).r;
        
        float3 N = normalize(input.normal);
        if (material.normalTexIndex != 0xFFFFFFFF)
            N = ApplyNormalMap(input.normal, input.tangent, normalMap);
        
        float3 V = normalize(cameraPosition - input.worldPos);
        float3 L = normalize(-lightDirection);
        
        float3 radiance = lightColor * lightIntensity;
        
        float3 finalColor = CalculatePBR(N, V, L, baseColor.rgb, metallic, roughness, radiance);
        
        float3 ambient = baseColor.rgb * 0.15;
        finalColor += ambient;
        
        finalColor = ApplyVolumetricFog(finalColor, input.worldPos, input.uv, cameraPosition);
        
        finalColor = DarkFantasyToneMapping(finalColor);
        
        return float4(finalColor, finalAlpha);
    }
    else
    {
        return input.color;
    }
}