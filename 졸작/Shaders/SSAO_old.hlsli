#include "ShaderResources.hlsli"

struct PS_IN
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
    float4 rt1 = gBufferRT1.Sample(linearSampler, input.uv);
    float4 rt2 = gBufferRT2.Sample(linearSampler, input.uv);
    
    float3 worldPos = rt2.xyz;
    float3 worldNormal = normalize(rt1.xyz);
    
    // AI HELPED
    // Background Check
    // If normal = 0 || WorldPos is too far ==> Background
    if (length(worldNormal) < 0.1f || length(worldPos) > 1000.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Calculate SSAO
    float ao = 0.0f;
    float radius = 0.5f;    // Sampling radius
    int sampleCount = 0;
    
    // 주변 픽셀 샘플링 (5x5 그리드)
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            if (x == 0 && y == 0)
                continue;
            
            // UV 오프셋 계산
            float2 offset = float2(x, y) * 0.001f; // 픽셀 단위
            float2 sampleUV = input.uv + offset;
            
            // 범위 체크
            if (sampleUV.x < 0.0f || sampleUV.x > 1.0f ||
                sampleUV.y < 0.0f || sampleUV.y > 1.0f)
                continue;
            
            // 주변 픽셀의 WorldPos 샘플링
            float3 samplePos = gBufferRT2.Sample(linearSampler, sampleUV).xyz;
            
            // 배경 픽셀 제외
            if (length(samplePos) > 1000.0f)
                continue;
            
            // 현재 픽셀과의 차이 벡터
            float3 diff = samplePos - worldPos;
            float distance = length(diff);
            
            float currentDepth = length(worldPos);
            float depthRatio = distance / currentDepth;
            
            if (depthRatio > 0.2f)
                continue;
            
            // 일정 범위 내의 픽셀만 체크
            if (distance < radius && distance > 0.01f)
            {
                // 방향 벡터
                float3 sampleDir = normalize(diff);
                
                // 노멀과의 각도 체크 (앞쪽에 있는 픽셀만)
                float normalDot = max(0.0f, dot(worldNormal, sampleDir));
                
                // AO 누적 (가까울수록, 노멀 방향일수록 강함)
                float attenuation = 1.0f - (distance / radius);
                ao += normalDot * attenuation;
                sampleCount++;
            }
        }
    }
    
    // === 4. AO 정규화 및 조절 ===
    if (sampleCount > 0)
        ao /= (float) sampleCount;
    
    // AO 강도 조절
    ao = saturate(ao * 1.0f); // 2.5f = 강도 (높을수록 어두움)
    
    // 최종 값 (1.0 = 밝음, 0.0 = 어두움)
    float finalAO = 1.0f - ao;
    
    // 최소값 보정 (너무 어두워지지 않게)
    finalAO = max(finalAO, 0.3f);
    
    return float4(finalAO, finalAO, finalAO, 1.0f);
}