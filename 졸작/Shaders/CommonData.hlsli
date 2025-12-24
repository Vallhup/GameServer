#ifndef COMMONDATA_HLSLI
#define COMMONDATA_HLSLI

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

struct AnimFrameParams
{
    float4 scale;
    float4 rotation;
    float4 translation;
};

float3 ApplyNormalMap(float3 worldNormal, float3 worldTangent, float3 normalMap)
{
    float3 N = normalize(worldNormal);
    float3 T = normalize(worldTangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(normalMap, TBN));
}

#endif