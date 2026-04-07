#ifndef SSAOPS_HLSLI
#define SSAOPS_HLSLI

#include "InOutFormats.hlsli"
#include "ShaderResources.hlsli"
#include "Constants.hlsli"

float3 ReconstructViewPos(float2 uv, float depth)
{
    float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 wp = mul(clipPos, invViewProj);
    float3 worldPos = wp.xyz / wp.w;
    
    return mul(float4(worldPos, 1.0f), view).xyz;
}

float4 PSMain(SSAO_PS_IN input) : SV_Target
{
    float depth = ssaoDepth.Sample(pointSampler, input.uv).r;
    
    if (depth >= 1.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    float3 fragPos = ReconstructViewPos(input.uv, depth);
    float3 worldNormal = normalize(ssaoNormal.Sample(pointSampler, input.uv).xyz);
    float3 viewNormal = normalize(mul(float4(worldNormal, 0.0f), view).xyz);
    
    float3 randomVec = ssaoNoise.Sample(pointSampler, input.uv * noiseScale).xyz;
    float3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    float3 biNormal = normalize(cross(viewNormal, tangent));
    float3x3 TBN = float3x3(tangent, biNormal, viewNormal);
    
    float occlusion = 0.0;
    
    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        float3 offset = mul(ssaoSamples[i].xyz, TBN);
        float3 sample = fragPos + offset * samplingRadius;
        
        float4 screenSample = mul(float4(sample, 1.0f), projection);
        screenSample.xyz /= screenSample.w;
        screenSample.xyz = screenSample.xyz * 0.5 + 0.5;
        screenSample.y = 1.0 - screenSample.y;
        
        float sampleDepth = ssaoDepth.Sample(pointSampler, screenSample.xy).r;
        float3 sampleViewPos = ReconstructViewPos(screenSample.xy, sampleDepth);
        
        // DirectX(Left-Handed)에서는 Z값이 작을수록 카메라에 가까움.
        // sampleViewPos.z <= sample.z - ssaoBias 일 때, 해당 기하구조가 샘플을 가리는 것.
        float rangeCheck = smoothstep(0.0, 1.0, samplingRadius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z <= sample.z - ssaoBias ? 1.0 : 0.0) * rangeCheck;
    }

    // occlusion은 '가려진 정도'를 의미하므로, 최종 AO(가려지지 않고 빛을 받는 정도)는 1.0에서 빼주어야 함.
    float ao = 1.0 - (occlusion / KERNEL_SIZE);
    
    // 먼 거리의 산 등에서 발생하는 깜빡임(Flickering) 및 정밀도 한계(Z-Fighting)를 해결하기 위해
    // 거리에 따라 SSAO 효과를 부드럽게 페이드아웃(Fade-out) 시킴.
    // 카메라로부터의 거리가 100.0 ~ 300.0 사이일 때 SSAO가 서서히 사라짐.
    float fadeStart = 100.0f;
    float fadeEnd = 300.0f;
    float distanceFade = smoothstep(fadeStart, fadeEnd, fragPos.z);
    
    ao = lerp(ao, 1.0f, distanceFade);
    
    return float4(ao, ao, ao, 1.0);
}

#endif