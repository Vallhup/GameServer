
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
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 albedo : SV_Target2;
};

Texture2D bindlessTextures[] : register(t0, space1);
StructuredBuffer<MaterialData> materialBuffer : register(t0);
SamplerState textureSampler : register(s0);

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

        // Bindless 텍스처 샘플링
        if (material.baseColorTexIndex != 0xFFFFFFFF)
        {
            baseColor = bindlessTextures[NonUniformResourceIndex(material.baseColorTexIndex)].Sample(textureSampler, input.uv);
        }

        output.position = input.worldPos;
        output.normal = float4(normalize(input.normal), 1.0f);
        output.albedo = baseColor;
    }
    else
    {
        output.position = input.worldPos;
        output.normal = float4(normalize(input.normal), 1.0f);
        output.albedo = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return output;
}