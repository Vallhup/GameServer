#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

SKYBOX_VS_OUT VSMain(SKYBOX_VS_IN input)
{
    SKYBOX_VS_OUT output;
    
    matrix viewNoTranslation = view;
    viewNoTranslation._41 = 0;
    viewNoTranslation._42 = 0;
    viewNoTranslation._43 = 0;
    
    float4 viewPos = mul(float4(input.pos, 1.0f), viewNoTranslation);
    output.pos = mul(viewPos, projection);
    output.pos.z = output.pos.w;
    output.localPos = input.pos;
    
    return output;
}