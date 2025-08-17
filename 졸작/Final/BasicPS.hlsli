
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
};

Texture2D bindlessTextures[] : register(t0, space1);
StructuredBuffer<MaterialData> materialBuffer : register(t0);
SamplerState textureSampler : register(s0);

float4 PSMain(PS_IN input) : SV_Target
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
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(textureSampler, input.uv);
        }
        
        if (material.normalTexIndex != 0xFFFFFFFF)
        {
            normalMap = bindlessTextures[NonUniformResourceIndex(material.normalTexIndex)].Sample(textureSampler, input.uv).rgb;
            normalMap = (normalMap - 0.5) * 2.0;
        }
        
        if (material.roughnessTexIndex != 0xFFFFFFFF)
        {
            roughness = bindlessTextures[NonUniformResourceIndex(material.roughnessTexIndex)].Sample(textureSampler, input.uv).r;
        }
        
        if (material.metallicTexIndex != 0xFFFFFFFF)
        {
            metallic = bindlessTextures[NonUniformResourceIndex(material.metallicTexIndex)].Sample(textureSampler, input.uv).r;
        }
        
        if (material.alphaTexIndex != 0xFFFFFFFF)
        {
            alpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].Sample(textureSampler, input.uv).a;
        }
        
        float3 lightDir = normalize(float3(0, 0, 1));
        float3 worldNormal = normalize(input.normal + normalMap * 0.3);
        float NdotL = max(0.0, dot(worldNormal, -lightDir));
        
        float3 diffuse = baseColor.rgb * NdotL * 0.7;
        float3 ambient = baseColor.rgb * 0.6;
        
        float3 viewDir = normalize(float3(0.1, 0.1, -1));
        float3 reflectDir = reflect(lightDir, worldNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * metallic * 0.3;
        
        float3 finalColor = diffuse + ambient + spec;
        
        return float4(finalColor, baseColor.a * alpha);
    }
    else
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}