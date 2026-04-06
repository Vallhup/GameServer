// Flame Effect Shader
// Additive blending with texture and time-based wobble

cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
    matrix invViewProj;
    float3 cameraPosition;
    float time;
    uint lutIndex;
    uint prevLutIndex;
    float lutBlendFactor;
    float saturationFactor;
};

cbuffer FlameCB : register(b13)
{
    float4 flameColor;
    uint textureIndex;
    float3 padding;
};

// Bindless Texture Array (Space 1)
Texture2D gTextures[] : register(t0, space1);
SamplerState gSampler : register(s0);

struct FlameVertexIn
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float alpha : ALPHA;
};

struct FlameVertexOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float alpha : ALPHA;
};

FlameVertexOut VSMain(FlameVertexIn input)
{
    FlameVertexOut output;
    
    float4 worldPos = float4(input.position, 1.0f);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    output.uv = input.uv;
    output.alpha = input.alpha;

    return output;
}

float4 PSMain(FlameVertexOut input) : SV_TARGET
{
    // 텍스처 샘플링 (스크롤 제거: 텍스처가 화면 밖으로 벗어나지 않게 고정)
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (textureIndex != 0xFFFFFFFF)
    {
        texColor = gTextures[textureIndex].Sample(gSampler, input.uv);
    }

    // 흑백 텍스처(Grayscale)이므로 R 채널의 값을 불꽃의 강도(Intensity)로 사용
    float flameIntensity = texColor.r;

    // 중앙은 밝게, 외곽은 자연스럽게 페이드 아웃 되도록 (X축 기반)
    float centerGlow = 1.0f - abs(input.uv.x - 0.5f) * 4.0f;

    // Y 위치에 따라 가로 폭 조절 (위/아래 얇게, 중간 넓게)
    float widthScale = smoothstep(0.0f, 0.3f, input.uv.y) * smoothstep(1.0f, 0.3f, input.uv.y);
    centerGlow = pow(centerGlow, lerp(7.0f, 1.5f, widthScale));  // 위아래는 pow 높게(좁게), 중간은 낮게(넓게)

    // 최종 알파 계산: 파티클 고유의 알파(수명 주기 페이드) * 텍스처 강도 * 보정값
    float finalAlpha = input.alpha * flameIntensity * centerGlow;

    // 3단계 그라데이션: 흰색(중심) → 노란색(중간) → 주황색(외곽)
    float3 white = float3(1.0f, 1.0f, 0.95f);
    float3 yellow = float3(1.0f, 0.85f, 0.25f);
    float3 orange = flameColor.rgb * float3(1.0f, 0.2f, 0.1f);  // 더 진한 주황

    float t = centerGlow * flameIntensity;
    float yellowBlend = smoothstep(0.0f, 0.99f, t);   // 주황 → 노랑
    float whiteBlend = pow(saturate(t), 7.0f);       // 노랑 → 흰색 (아주 중심부만)

    float3 baseColor = lerp(orange, yellow, yellowBlend);
    baseColor = lerp(baseColor, white, whiteBlend);
    float3 color = baseColor * flameIntensity * (1.0f + centerGlow * 0.5f);

    return float4(color, finalAlpha * flameColor.a);
}