// ============================================================================
// Orthodox SSAO Shader (Frank Luna Method)
// ============================================================================

#include "ShaderResources.hlsli"

// View Space G-Buffer
Texture2D gViewNormal : register(t10); // View Normal
Texture2D gViewPosition : register(t11); // View Position
Texture2D gRandomVec : register(t12); // Random Vectors

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

// ============================================================================
// Vertex Shader (Fullscreen Quad)
// ============================================================================

VertexOut VSMain(uint vertexID : SV_VertexID)
{
    VertexOut output;
    
    // Fullscreen Quad UV °è»ê
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    
    // UV [0,1] ¡æ NDC [-1,1]
    output.position = float4(
        output.uv.x * 2.0 - 1.0,
        1.0 - output.uv.y * 2.0,
        0.0,
        1.0
    );
    
    return output;
}

// ============================================================================
// Occlusion Function (Frank Luna)
// ============================================================================

float OcclusionFunction(float distZ)
{
    // Fade out based on distance
    float occlusion = 0.0;
    
    if (distZ > surfaceEpsilon)
    {
        float fadeLength = occlusionFadeEnd - occlusionFadeStart;
        
        // Linear fade
        occlusion = saturate((occlusionFadeEnd - distZ) / fadeLength);
    }
    
    return occlusion;
}

// ============================================================================
// Pixel Shader (SSAO Calculation)
// ============================================================================

float PSMain(VertexOut input) : SV_TARGET
{
    float3 viewNormal = gViewNormal.Sample(pointSampler, input.uv).xyz;
    float3 viewPos = gViewPosition.Sample(pointSampler, input.uv).xyz;
    
    if (abs(viewPos.z) < 0.0001f)
        return 1.0f;
    
    float2 randomUV = input.uv * float2(2560.0 / 256.0, 1440.0 / 256.0);
    float3 randomVec = gRandomVec.Sample(linearSampler, randomUV).xyz;
    randomVec = randomVec * 2.0 - 1.0;
    
    float3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    float3 bitangent = cross(viewNormal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, viewNormal);
    
    float occlusionSum = 0.0;
    
    for (int i = 0; i < 14; ++i)
    {
        float3 offset = mul(ssaoOffsetVectors[i].xyz, TBN);
        float3 samplePos = viewPos + offset * occlusionRadius;
        
        float4 sampleClip = mul(float4(samplePos, 1.0), ssaoProjection);
        sampleClip.xyz /= sampleClip.w;
        
        float2 sampleUV = sampleClip.xy * 0.5 + 0.5;
        sampleUV.y = 1.0 - sampleUV.y;
        
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;
        
        float3 sampleViewPos = gViewPosition.Sample(pointSampler, sampleUV).xyz;
        
        float distZ = sampleViewPos.z - viewPos.z;
        float dp = max(dot(viewNormal, normalize(sampleViewPos - viewPos)), 0.0);
        float occlusion = dp * OcclusionFunction(distZ);
        
        occlusionSum += occlusion;
    }
    
    occlusionSum /= 14.0;
    float accessibility = 1.0 - occlusionSum;
    
    return saturate(pow(accessibility, 2.0));
}