#include "Shadow.hlsli"
#include "InOutFormats.hlsli"
#include "PBR.hlsli"
#include "Fog.hlsli"

// D3D12 standard projection (LH, NDC z 0~1) 의 ndcZ → viewZ 역산
float LinearizeDepth(float ndcZ, float zNear, float zFar)
{
    return (zFar * zNear) / (zFar - ndcZ * (zFar - zNear));
}

float4 PSMain(LIGHTING_PS_IN input) : SV_Target
{
    float4 rt0 = gBufferRT0.Sample(pointSampler, input.uv);
    float4 rt1 = gBufferRT1.Sample(pointSampler, input.uv);
    float4 rt2 = gBufferRT2.Sample(pointSampler, input.uv);
    float depth = depthBuffer.Sample(pointSampler, input.uv).r;

    if (depth >= 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    float3 baseColor = rt0.rgb;
    float metallic = rt0.a;

    float3 worldNormal = normalize(rt1.xyz);
    float roughness = rt1.w;

    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 wp = mul(clipPos, invViewProj);
    float3 worldPos = wp.xyz / wp.w;

    float3 emission = rt2.rgb;
    float ao = rt2.a;

    float3 N = worldNormal;
    float3 V = normalize(cameraPosition - worldPos);

    // ----- cluster lookup (4단계 CS 의 clusterIdx 산식과 정확히 일치해야 함) -----
    float linearD    = LinearizeDepth(depth, clusterZNear, clusterZFar);
    float sliceFloat = log(linearD) * clusterSliceScale + clusterSliceBias;
    uint  sliceIdx   = uint(max(0.0, sliceFloat));
    sliceIdx = min(sliceIdx, clusterGridDims.z - 1u);

    uint2 tile = uint2(input.uv * float2(clusterGridDims.xy));
    tile = min(tile, clusterGridDims.xy - uint2(1, 1));

    uint clusterIdx = sliceIdx * clusterGridDims.x * clusterGridDims.y
                    + tile.y  * clusterGridDims.x
                    + tile.x;

    uint2 listEntry = clusterLightGrid[clusterIdx];
    uint  offset    = listEntry.x;
    uint  count     = listEntry.y;

    float3 directLight = float3(0, 0, 0);
    float  shadow      = 1.0;

    for (uint n = 0; n < count; ++n)
    {
        uint i = clusterLightIndices[offset + n];

        if (lights[i].intensity <= 0.0)
            continue;

        float3 lightContribution = float3(0, 0, 0);

        if (lights[i].type == 0) // Directional Light
        {
            float3 L = normalize(-lights[i].position);
            float3 radiance = lights[i].color * lights[i].intensity;

            lightContribution = CalculateCurrentPBR(N, V, L, baseColor, metallic, roughness, radiance);

            if (i == 0)
            {
                float viewDepth = length(worldPos - cameraPosition);
                shadow = CalculateShadow(worldPos, N, viewDepth);
                lightContribution *= shadow;
            }
        }
        else if (lights[i].type == 1) // Point Light
        {
            float3 lightVec = lights[i].position - worldPos;
            float distance = length(lightVec);

            if (distance < lights[i].range)
            {
                float3 L = normalize(lightVec);
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation;

                float3 radiance = lights[i].color * lights[i].intensity * attenuation;

                lightContribution = CalculateCurrentPBR(N, V, L, baseColor, metallic, roughness, radiance);
            }
        }

        directLight += lightContribution;
    }

    float ssao = ssaoTexture.Sample(linearSampler, input.uv).r;
    ssao = lerp(1.0, ssao, 0.5);
    
    float finalAO = ao * ssao;  // 추후에 어떻게 진행할지 생각 필요, 다찬이와 논의
    
    float3 iblAmbient = CalculateIBL(
        N, V, baseColor, metallic, roughness, finalAO,
        bindlessCubeMaps[NonUniformResourceIndex(skyIrrIdx)],    // irradiance
        bindlessCubeMaps[NonUniformResourceIndex(skyRadIdx)],    // radiance
        bindlessTextures[NonUniformResourceIndex(BRDF_LUT_INDEX)],          // BRDF LUT
        linearSampler
    );

    iblAmbient *= lerp(0.8, 1.0, shadow);
    
    float3 finalColor = directLight + iblAmbient + emission;

    float4 fog = fogTexture.Sample(linearSampler, input.uv);
    finalColor = finalColor * fog.a + fog.rgb;

    return float4(finalColor, 1.0);
}