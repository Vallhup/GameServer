#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"
#include "PBR.hlsli"
#include "Fog.hlsli"

float4 PSMain(FORWARD_PS_IN input) : SV_Target
{
    if (useTexture == 1)
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
        float3 L = normalize(-lights[0].position);

        float3 radiance = lights[0].color * lights[0].intensity;

        float3 finalColor = CalculateCurrentPBR(N, V, L, baseColor.rgb, metallic, roughness, radiance);

        float3 ambient = baseColor.rgb * 0.15;
        finalColor += ambient;
        
        float2 screenUV = input.pos.xy * vfTexelSize;
        float4 fog = fogTexture.Sample(linearSampler, screenUV);
        finalColor = finalColor * fog.a + fog.rgb;

        return float4(finalColor, finalAlpha);
    }
    else if (useTexture == 2)
    {
        // Water rendering
        float2 uv = input.uv;
        
        // Simple wave displacement
        uv.x += sin(uv.y * 10.0f + waterTime * waveSpeed * 10.0f) * waveStrength;
        uv.y += cos(uv.x * 10.0f + waterTime * waveSpeed * 10.0f) * waveStrength;
        
        // Procedural normal for water - 출렁임을 더 강조하기 위해 빈도와 스케일 증가
        float3 N = normalize(input.normal);
        float3 tangent = normalize(input.tangent);
        float3 bitangent = cross(N, tangent);
        
        float waveNormalX = sin(uv.x * 30.0f + waterTime * waveSpeed * 8.0f) * waveStrength * 8.0f;
        float waveNormalY = cos(uv.y * 30.0f + waterTime * waveSpeed * 8.0f) * waveStrength * 8.0f;
        
        float3 normalMap = normalize(float3(waveNormalX, waveNormalY, 1.0f));
        N = normalize(mul(normalMap, float3x3(tangent, bitangent, N)));
        
        float3 V = normalize(cameraPosition - input.worldPos);
        float3 L = normalize(-lights[0].position);
        float3 radiance = lights[0].color * lights[0].intensity;

        float roughness = 0.05f; // 물 표면을 더 매끄럽게 해서 반사를 날카롭게 만듦
        float metallic = 0.2f;
        
        float3 finalColor = CalculateCurrentPBR(N, V, L, waterColor.rgb, metallic, roughness, radiance);
        
        float3 ambient = waterColor.rgb * 0.35f; // 기본 밝기(Ambient)를 크게 올려 호수를 밝게 만듦
        finalColor += ambient;
        
        float2 screenUV = input.pos.xy * vfTexelSize;
        float4 fog = fogTexture.Sample(linearSampler, screenUV);
        finalColor = finalColor * fog.a + fog.rgb;

        return float4(finalColor, waterColor.a);
    }
    else
    {
        return input.color;
    }
}