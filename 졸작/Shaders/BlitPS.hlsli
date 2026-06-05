#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"
#include "Constants.hlsli"
#include "ToneMapping.hlsli"

float4 PSMain(FULLSCREEN_VS_OUT input) : SV_Target
{
    float3 hdr   = bindlessTextures[HDR_SCENE_BINDLESS_INDEX].Sample(pointSampler, input.uv).rgb;
    float3 bloom = bindlessTextures[BLOOM_MIP_BASE].Sample(linearClampSampler, input.uv).rgb;

    //float3 debugBloom = lerp(float3(0, 0, 0), bloom, saturate(bloomIntensity));
    //debugBloom = DarkFantasyToneMapping(debugBloom, saturationFactor);

    //if (lutIndex != 0xFFFFFFFF)
    //{
    //    if (lutBlendFactor >= 1.0)
    //        debugBloom = ApplyLUT(bindlessTextures3D[lutIndex], linearClampSampler, debugBloom);
    //    else
    //        debugBloom = ApplyLUTCircle(
    //            bindlessTextures3D[lutIndex],
    //            bindlessTextures3D[prevLutIndex],
    //            linearClampSampler, debugBloom, lutBlendFactor, input.uv);
    //}
    //return float4(debugBloom, 1.0);
    
    // Progressive bloom composite (pre-tonemap). bloomIntensity comes from BloomCB (b13).
    //float3 finalColor = lerp(hdr, bloom, saturate(bloomIntensity));

    // Traditional bright-pass + additive bloom. bloomIntensity from BloomCB (b13).
    float3 finalColor = hdr + bloom * bloomIntensity;
    
    finalColor = DarkFantasyToneMapping(finalColor, saturationFactor);

    if (lutIndex != 0xFFFFFFFF)
    {
        if (lutBlendFactor >= 1.0)
            finalColor = ApplyLUT(bindlessTextures3D[lutIndex], linearClampSampler, finalColor);
        else
            finalColor = ApplyLUTWithTransition(
                bindlessTextures3D[lutIndex],
                bindlessTextures3D[prevLutIndex],
                linearClampSampler, finalColor, lutBlendFactor);
    }

    finalColor *= screenBrightness;

    return float4(finalColor, 1.0);
}
