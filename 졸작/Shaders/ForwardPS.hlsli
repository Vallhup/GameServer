#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

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
        
        // Bindless 텍스처 샘플링
        if (material.baseColorTexIndex != 0xFFFFFFFF)
        {
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(linearSampler, input.uv);
        }
        
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(linearSampler, input.uv).rgb;
            normalMap = (normalMap - 0.5) * 2.0;
        }
        
        if (material.roughnessTexIndex != 0xFFFFFFFF)
        {
            roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(linearSampler, input.uv).r;
        }
        
        if (material.metallicTexIndex != 0xFFFFFFFF)
        {
            metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(linearSampler, input.uv).r;
        }
        
        if (material.alphaTexIndex != 0xFFFFFFFF)
        {
            alpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(linearSampler, input.uv).a;
        }
        
        float3 lightDir = normalize(-lightDirection);
        
        float3 worldNormal = normalize(input.normal);
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            float3 N = worldNormal;
            float3 T = normalize(input.tangent);
            float3 B = cross(N, T);
            
            float3x3 TBN = float3x3(T, B, N);
            
            float normalStrength = 1.0f;
            float3 tangentNormal = float3(normalMap.x * normalStrength, normalMap.y * normalStrength, normalMap.z);
            tangentNormal = normalize(tangentNormal);
            worldNormal = normalize(mul(tangentNormal, TBN));
        }
        
        float NdotL = max(0.0, dot(worldNormal, -lightDir));
        
        float3 diffuse = baseColor.rgb * NdotL * 0.7;
        float3 ambient = baseColor.rgb * 0.3;
        
        float3 viewDir = normalize(float3(0.1, 0.1, -1));
        float3 reflectDir = reflect(lightDir, worldNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.01;
        
        float3 finalColor = (diffuse + ambient + spec) * lightColor * lightIntensity;
        
        return float4(finalColor, baseColor.a * alpha);
    }
    else
    {
        return input.color;
    }
}