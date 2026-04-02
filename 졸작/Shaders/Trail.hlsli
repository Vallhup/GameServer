// Trail Effect Shader
// Additive blending으로 칼 잔상 효과 렌더링

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

cbuffer TrailCB : register(b13)
{
    float4 trailColor;
};

struct TrailVertexIn
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float alpha : ALPHA;
};

struct TrailVertexOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float alpha : ALPHA;
};

TrailVertexOut VSMain(TrailVertexIn input)
{
    TrailVertexOut output;

    // World -> View -> Projection
    float4 worldPos = float4(input.position, 1.0f);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    output.uv = input.uv;
    output.alpha = input.alpha;

    return output;
}

float4 PSMain(TrailVertexOut input) : SV_TARGET
{
    // UV.x에 따른 그라데이션 (꼬리 → 머리)
    float gradientFactor = input.uv.x;

    // 중앙이 밝고 가장자리가 어두운 효과 (UV.y 기반)
    float centerGlow = 1.0f - abs(input.uv.y - 0.5f) * 2.0f;
    centerGlow = pow(centerGlow, 0.5f);  // 부드러운 페이드

    // 최종 알파 계산
    float finalAlpha = input.alpha * centerGlow * gradientFactor;

    // 색상에 글로우 효과 추가
    float3 color = trailColor.rgb * (1.0f + centerGlow * 0.5f);

    return float4(color, finalAlpha * trailColor.a);
}
