#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

// 3x3 tent filter upsample
//   1 2 1
//   2 4 2  * (1/16)
//   1 2 1
// Samples src mip at radius = bloomFilterRadius (in UV space).
// Blended ADDITIVELY into the destination larger mip via PSO blend state.

float4 PSMain(FULLSCREEN_VS_OUT input) : SV_Target
{
    float2 uv = input.uv;
    float r = bloomFilterRadius;

    Texture2D src = bindlessTextures[bloomSrcMipIndex];

    float3 a = src.Sample(linearClampSampler, uv + float2(-r, -r)).rgb;
    float3 b = src.Sample(linearClampSampler, uv + float2( 0, -r)).rgb;
    float3 c = src.Sample(linearClampSampler, uv + float2( r, -r)).rgb;

    float3 d = src.Sample(linearClampSampler, uv + float2(-r,  0)).rgb;
    float3 e = src.Sample(linearClampSampler, uv + float2( 0,  0)).rgb;
    float3 f = src.Sample(linearClampSampler, uv + float2( r,  0)).rgb;

    float3 g = src.Sample(linearClampSampler, uv + float2(-r,  r)).rgb;
    float3 h = src.Sample(linearClampSampler, uv + float2( 0,  r)).rgb;
    float3 i = src.Sample(linearClampSampler, uv + float2( r,  r)).rgb;

    float3 color = e * 4.0
                 + (b + d + f + h) * 2.0
                 + (a + c + g + i);
    color *= (1.0 / 16.0);

    return float4(color, 1.0);
}
