#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"
#include "Constants.hlsli"
#include "ToneMapping.hlsli"

float4 PSMain(FULLSCREEN_VS_OUT input) : SV_Target
{
    float3 finalColor = bindlessTextures[HDR_SCENE_BINDLESS_INDEX].Sample(pointSampler, input.uv).rgb;

    finalColor = DarkFantasyToneMapping(finalColor, saturationFactor);

    if (lutIndex != 0xFFFFFFFF)
    {
        if (lutBlendFactor >= 1.0)
            finalColor = ApplyLUT(bindlessTextures3D[lutIndex], lutLinearSampler, finalColor);
        else
            finalColor = ApplyLUTCircle(
                bindlessTextures3D[lutIndex],
                bindlessTextures3D[prevLutIndex],
                lutLinearSampler, finalColor, lutBlendFactor, input.uv);
    }

    return float4(finalColor, 1.0);
}
