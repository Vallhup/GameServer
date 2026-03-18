#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI

float3 PBRNeutralToneMapping(float3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;
    
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    
    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
        return color;
    
    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    
    color *= newPeak / peak;
    
    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    
    return lerp(color, newPeak * float3(1, 1, 1), g);
}

// ACES Filmic Tone Mapping
// 콘트라스트 높고, 어두운 부분 더 어둡게, 영화 같은 느낌
float3 ACESFilmicToneMapping(float3 color)
{
    // ACES input matrix (sRGB -> ACES)
    const float3x3 inputMat = float3x3(
          0.59719, 0.35458, 0.04823,
          0.07600, 0.90834, 0.01566,
          0.02840, 0.13383, 0.83777
    );

    // ACES output matrix (ACES -> sRGB)
    const float3x3 outputMat = float3x3(
          1.60475, -0.53108, -0.07367,
          -0.10208, 1.10813, -0.00605,
          -0.00327, -0.07276, 1.07602
    );

    float exposure = 1.2;
    float contrast = 1.15;
    
    color *= exposure;
    color = pow(color, contrast);
    
    color = mul(inputMat, color);

    // RRT and ODT fit
    float3 a = color * (color + 0.0245786) - 0.000090537;
    float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;

    color = mul(outputMat, color);

    return saturate(color);
}

float3 Uncharted2ToneMapping(float3 color)
{
    float A = 0.15; // Shoulder Strength
    float B = 0.50; // Linear Strength
    float C = 0.10; // Linear Angle
    float D = 0.20; // Toe Strength
    float E = 0.02; // Toe Numerator
    float F = 0.30; // Toe Denominator
    float W = 11.2; // White Point

    float exposure = 2.0;
    color *= exposure;

    float3 curr = ((color * (A * color + C * B) + D * E) /
                     (color * (A * color + B) + D * F)) - E / F;

    float3 whiteScale = 1.0 / (((W * (A * W + C * B) + D * E) /
                                  (W * (A * W + B) + D * F)) - E / F);

    return curr * whiteScale;
}

float3 DarkFantasyToneMapping(float3 color)
{
    // 1. ACES Filmic
    const float3x3 inputMat = float3x3(
          0.59719, 0.35458, 0.04823,
          0.07600, 0.90834, 0.01566,
          0.02840, 0.13383, 0.83777
    );
    
    const float3x3 outputMat = float3x3(
          1.60475, -0.53108, -0.07367,
          -0.10208, 1.10813, -0.00605,
          -0.00327, -0.07276, 1.07602
    );

    color = mul(inputMat, color);
    float3 a = color * (color + 0.0245786) - 0.000090537;
    float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;
    color = mul(outputMat, color);
    color = saturate(color);

    // 2. 다크 판타지 후보정
    float contrast = 1.2;
    float saturation = 0.85; // 살짝 desaturate (0.85 / 0.95 / 1.15 / 1.35)
    float3 shadowTint = float3(0.9, 0.9, 1.1); // 그림자에 차가운 톤

    // 콘트라스트
    color = pow(color, contrast);

    // 채도 조절
    float luma = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(float3(luma, luma, luma), color, saturation);

    // 어두운 부분에 차가운 틴트
    float shadowMask = 1.0 - luma;
    color *= lerp(float3(1, 1, 1), shadowTint, shadowMask * 0.3);

    return saturate(color);
}

float3 ApplyLUT(Texture3D lutTex, SamplerState samp, float3 color)
{
    const float LUT_SIZE = 32.0;
    float scale = (LUT_SIZE - 1.0) / LUT_SIZE;
    float offset = 0.5 / LUT_SIZE;

    float3 lutCoord = saturate(color) * scale + offset;
    return lutTex.Sample(samp, lutCoord).rgb;
}

#endif