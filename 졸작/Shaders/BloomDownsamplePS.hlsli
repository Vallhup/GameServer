#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

// Jimenez 13-tap partial Karis average downsample
// - 4 inner 2x2 block averages (partial Karis for firefly suppression)
// - 4 corner taps, 4 edge taps, 1 center tap for the outer ring
// Reference: "Next Generation Post Processing in Call of Duty: Advanced Warfare"

float KarisWeight(float3 c)
{
    float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
    return 1.0 / (1.0 + luma);
}

float3 KarisAverage(float3 c0, float3 c1, float3 c2, float3 c3)
{
    float w0 = KarisWeight(c0);
    float w1 = KarisWeight(c1);
    float w2 = KarisWeight(c2);
    float w3 = KarisWeight(c3);
    float wSum = w0 + w1 + w2 + w3;
    return (c0 * w0 + c1 * w1 + c2 * w2 + c3 * w3) / max(wSum, 1e-5);
}

// UE/Unity-style soft-knee prefilter.
// luma < threshold - knee  -> contribution ~ 0
// luma ~ threshold         -> smooth quadratic ramp of width 2*knee
// luma > threshold + knee  -> near full contribution
float3 SoftKneePrefilter(float3 color, float threshold, float knee)
{
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));

    float soft = luma - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-5);

    float contribution = max(luma - threshold, soft) / max(luma, 1e-5);
    return color * contribution;
}

float4 PSMain(FULLSCREEN_VS_OUT input) : SV_Target
{
    float2 uv = input.uv;
    float2 tx = bloomSrcTexelSize;

    Texture2D src = bindlessTextures[bloomSrcMipIndex];

    // 13 taps around current pixel (named by offset-from-center in texels)
    // Outer ring (corners + edges): spacing 2 texels
    float3 A = src.Sample(linearClampSampler, uv + tx * float2(-2, -2)).rgb;
    float3 B = src.Sample(linearClampSampler, uv + tx * float2( 0, -2)).rgb;
    float3 C = src.Sample(linearClampSampler, uv + tx * float2( 2, -2)).rgb;
    float3 D = src.Sample(linearClampSampler, uv + tx * float2(-2,  0)).rgb;
    float3 E = src.Sample(linearClampSampler, uv + tx * float2( 0,  0)).rgb;
    float3 F = src.Sample(linearClampSampler, uv + tx * float2( 2,  0)).rgb;
    float3 G = src.Sample(linearClampSampler, uv + tx * float2(-2,  2)).rgb;
    float3 H = src.Sample(linearClampSampler, uv + tx * float2( 0,  2)).rgb;
    float3 I = src.Sample(linearClampSampler, uv + tx * float2( 2,  2)).rgb;

    // Inner ring (2x2 blocks), spacing 1 texel
    float3 J = src.Sample(linearClampSampler, uv + tx * float2(-1, -1)).rgb;
    float3 K = src.Sample(linearClampSampler, uv + tx * float2( 1, -1)).rgb;
    float3 L = src.Sample(linearClampSampler, uv + tx * float2(-1,  1)).rgb;
    float3 M = src.Sample(linearClampSampler, uv + tx * float2( 1,  1)).rgb;

    // First pass only: prefilter HDR scene to keep dark areas out of the mip-chain.
    if (bloomIsFirstPass != 0)
    {
        A = SoftKneePrefilter(A, bloomThreshold, bloomKnee);
        B = SoftKneePrefilter(B, bloomThreshold, bloomKnee);
        C = SoftKneePrefilter(C, bloomThreshold, bloomKnee);
        D = SoftKneePrefilter(D, bloomThreshold, bloomKnee);
        E = SoftKneePrefilter(E, bloomThreshold, bloomKnee);
        F = SoftKneePrefilter(F, bloomThreshold, bloomKnee);
        G = SoftKneePrefilter(G, bloomThreshold, bloomKnee);
        H = SoftKneePrefilter(H, bloomThreshold, bloomKnee);
        I = SoftKneePrefilter(I, bloomThreshold, bloomKnee);
        J = SoftKneePrefilter(J, bloomThreshold, bloomKnee);
        K = SoftKneePrefilter(K, bloomThreshold, bloomKnee);
        L = SoftKneePrefilter(L, bloomThreshold, bloomKnee);
        M = SoftKneePrefilter(M, bloomThreshold, bloomKnee);
    }

    // 5 overlapping 2x2 groups
    //   innerBox : J, K, L, M           (weight 0.5 / 4 pts = 0.125 each)
    //   topLeft  : A, B, D, E           (weight 0.125 / 4 pts = 0.03125 each)
    //   topRight : B, C, E, F
    //   botLeft  : D, E, G, H
    //   botRight : E, F, H, I

    // partial Karis average for the 5 groups
    float3 innerBox = KarisAverage(J, K, L, M);
    float3 tl       = KarisAverage(A, B, D, E);
    float3 tr       = KarisAverage(B, C, E, F);
    float3 bl       = KarisAverage(D, E, G, H);
    float3 br       = KarisAverage(E, F, H, I);

    float3 color = innerBox * 0.5
                 + (tl + tr + bl + br) * 0.125;

    return float4(max(color, 0.0), 1.0);
}
