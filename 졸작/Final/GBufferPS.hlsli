
cbuffer ObjectCB : register(b1)
{
    matrix world;
    int useTexture;
    int useInstancing;
    int hasAlpha;
    uint materialIndex;
};

struct MaterialData
{
    uint baseColorTexIndex;
    uint normalTexIndex;
    uint roughnessTexIndex;
    uint metallicTexIndex;
    uint heightTexIndex;
    uint alphaTexIndex;
    uint emissionTexIndex;
    uint aoTexIndex;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weights : WEIGHT;
    float4 indices : INDICES;
    float4 color : COLOR;
    uint materialIndex : MATERIAL_INDEX;
    float4 worldPos : POSITION;
};

struct PS_OUT
{
    float4 RT0 : SV_Target0; // BaseColor.rgb + Metallic.r
    float4 RT1 : SV_Target1; // Normal.xyz + Roughness.r  
    float4 RT2 : SV_Target2; // WorldPos.xyz + AO.r
    float4 RT3 : SV_Target3; // Emission.rgb + Alpha.r (또는 MaterialID)
};

Texture2D bindlessTextures[] : register(t0, space1);
StructuredBuffer<MaterialData> materialBuffer : register(t0);
SamplerState textureSampler : register(s0);

float3 ApplyNormalMap(float3 worldNormal, float3 worldTangent, float3 normalMap)
{
    float3 N = normalize(worldNormal);
    float3 T = normalize(worldTangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(normalMap, TBN));
}

PS_OUT PSMain(PS_IN input) : SV_Target
{
    PS_OUT output;

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
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(textureSampler, input.uv);
        
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(textureSampler, input.uv).rgb;
            normalMap = (normalMap - 0.5) * 2.0;
        }
        
        if (material.roughnessTexIndex != 0xFFFFFFFF)
            roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(textureSampler, input.uv).r;
                
        if (material.metallicTexIndex != 0xFFFFFFFF)
            metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(textureSampler, input.uv).r;
               
        if (material.alphaTexIndex != 0xFFFFFFFF)
            alpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(textureSampler, input.uv).a;
        
        if (material.emissionTexIndex != 0xFFFFFFFF)
            emission = bindlessTextures[NonUniformResourceIndex(material.emissionTexIndex)].Sample(textureSampler, input.uv).rgb;
        
        if (material.aoTexIndex != 0xFFFFFFFF)
            ao = bindlessTextures[NonUniformResourceIndex(material.aoTexIndex)].Sample(textureSampler, input.uv).r;
        
        if (material.heightTexIndex != 0xFFFFFFFF)
            height = bindlessTextures[NonUniformResourceIndex(material.heightTexIndex)].Sample(textureSampler, input.uv).r;
        
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
