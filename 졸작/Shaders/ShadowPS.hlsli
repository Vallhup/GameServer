#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

void PSMain(SHADOW_PS_IN input)
{
    MaterialData material = materialBuffer[input.materialIndex];
    
    float finalAlpha = 1.0f;
    
    if (material.alphaTexIndex != 0xFFFFFFFF)
    {
        finalAlpha = bindlessTextures[NonUniformResourceIndex(material.alphaTexIndex)].SampleLevel(linearSampler, input.uv, 0).r;
    }

    clip(finalAlpha - 0.01f);
    
    if (dissolveAmount > 0.0f && dissolveNoiseIndex != 0xFFFFFFFF)
    {
        float n = bindlessTextures[NonUniformResourceIndex(dissolveNoiseIndex)].SampleLevel(linearSampler, input.uv, 0).r;
        clip(n - dissolveAmount);
    }
}