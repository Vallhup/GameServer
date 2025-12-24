#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

GBUFFER_PS_OUT PSMain(GBUFFER_PS_IN input) : SV_Target
{
    GBUFFER_PS_OUT output;

    if (useTexture)
    {
        MaterialData material = materialBuffer[input.materialIndex];

        float4 baseColor = float4(1, 1, 1, 1);
        float3 normalMap = float3(0, 0, 1);
        float roughness = 0.5f;
        float metallic = 0.0f;
        float alpha = 1.0f;
        float ao = 1.0f;
        float3 emission = float3(0, 0, 0);
        float height = 0.0f;
        
        // Bindless 텍스처 샘플링
        if (material.baseColorTexIndex != 0xFFFFFFFF)
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(linearSampler, input.uv);
        
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(linearSampler, input.uv).rgb;
            normalMap = (normalMap - 0.5) * 2.0;
        }
        
        if (material.roughnessTexIndex != 0xFFFFFFFF)
            roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(linearSampler, input.uv).r;
                
        if (material.metallicTexIndex != 0xFFFFFFFF)
            metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(linearSampler, input.uv).r;
               
        if (material.alphaTexIndex != 0xFFFFFFFF)
            alpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(linearSampler, input.uv).a;
        
        if (material.emissionTexIndex != 0xFFFFFFFF)
            emission = bindlessTextures[NonUniformResourceIndex(material.emissionTexIndex)].Sample(linearSampler, input.uv).rgb;
        
        if (material.aoTexIndex != 0xFFFFFFFF)
            ao = bindlessTextures[NonUniformResourceIndex(material.aoTexIndex)].Sample(linearSampler, input.uv).r;
        
        if (material.heightTexIndex != 0xFFFFFFFF)
            height = bindlessTextures[NonUniformResourceIndex(material.heightTexIndex)].Sample(linearSampler, input.uv).r;
        
        float3 worldNormal = ApplyNormalMap(input.normal, input.tangent, normalMap);
        
        output.RT0 = float4(baseColor.rgb, metallic);
        output.RT1 = float4(worldNormal, roughness);
        output.RT2 = float4(input.worldPos.xyz, ao);
        output.RT3 = float4(emission, alpha);
    }
    else
    {
        output.RT0 = float4(1, 0, 0, 0);
        output.RT1 = float4(normalize(input.normal), 0.8);
        output.RT2 = float4(input.worldPos.xyz, 1.0);
        output.RT3 = float4(0, 0, 0, 1);
    }
    
    return output;
}
