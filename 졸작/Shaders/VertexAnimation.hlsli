#ifndef VERTEXANIMATION_HLSLI
#define VERTEXANIMATION_HLSLI

void VertexAnimation(inout float3 pos, matrix mat, float4 color, float time)
{
    float3 worldPosTemp = mul(float4(pos, 1.0f), mat).xyz;

    float windSpeed = 2.0f;
    float windScale = 0.2f;

    // Primary wave (큰 움직임)
    float primaryWave = sin(time * windSpeed + worldPosTemp.x * 0.1f + worldPosTemp.z * 0.05f);

    // Secondary wave (디테일)
    float secondaryWave = sin(time * windSpeed * 2.0f + worldPosTemp.x * 0.3f + worldPosTemp.z * 0.2f) * 0.5f;

    float windOffset = (primaryWave + secondaryWave) * windScale;

    // 버텍스 컬러 R = 흔들림 강도 (뿌리=0, 끝=1)
    pos.x += color.r * windOffset;
    pos.z += color.r * windOffset * 0.5f;
}

#endif