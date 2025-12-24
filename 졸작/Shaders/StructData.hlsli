#ifndef STRUCTDATA_HLSLI
#define STRUCTDATA_HLSLI

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

#endif