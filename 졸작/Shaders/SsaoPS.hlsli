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
        
        float rangeCheck = smoothstep(0.0, 1.0, samplingRadius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= sample.z + ssaoBias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = occlusion / KERNEL_SIZE;
    return float4(ao, ao, ao, 1.0);
}

#endif