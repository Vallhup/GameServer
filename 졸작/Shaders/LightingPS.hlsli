#include "Shadow.hlsli"
#include "InOutFormats.hlsli"
#include "PBR.hlsli"

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
    
    if (ao > 1.5f)
    {
        float3 unlitColor = emission;
        float4 unlitFog = fogTexture.Sample(linearSampler, input.uv);
        unlitColor = unlitColor * unlitFog.a + unlitFog.rgb;
        return float4(unlitColor, 1.0);
    }

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
                shadow = lerp(shadowFloor, 1.0, shadow);
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

                // 정적 그림자 (베이크된 cube array, slice = i-1)
                if (i >= 1 && float(i - 1) < pointShadowCount)
                {
                    float ps = SamplePointShadow(worldPos, N, lights[i].position, lights[i].range, i - 1);
                    lightContribution *= lerp(1.0 - pointShadowStrength, 1.0, ps);
                }
            }
        }

        directLight += lightContribution;
    }

    // 실내(성당): 태양이 꺼져 있어 위 shadow 경로가 안 도므로, overhead 그림자를
    // point light 누적분에 직접 곱한다. ambient는 아래 공통 라인에서 같은 shadow로 처리.
    if (overheadMode > 0.5)
    {
        // 위를 향한 면(바닥)에만 그림자. 캐릭터 피부/몸은 노멀이 옆이라 걸러짐.
        float floorMask = smoothstep(0.6, 0.85, N.y);
        float s = SampleOverheadShadow(worldPos, N);
        // 강도로 부분 차폐. strength<1이면 빛이 0까지 안 떨어져 다른 맵처럼 옅게 유지.
        float occ = (1.0 - s) * floorMask * overheadStrength;   // 0=밝음 .. 1=완전 그림자
        shadow = 1.0 - occ;
        directLight *= shadow;
    }

    float ssao = ssaoTexture.Sample(linearSampler, input.uv).r;
    ssao = lerp(1.0, ssao, 0.5);
    
    float finalAO = ao * ssao;  
    
    float3 iblAmbient = CalculateIBL(
        N, V, baseColor, metallic, roughness, finalAO,
        bindlessCubeMaps[NonUniformResourceIndex(skyIrrIdx)],    // irradiance
        bindlessCubeMaps[NonUniformResourceIndex(skyRadIdx)],    // radiance
        bindlessTextures[NonUniformResourceIndex(BRDF_LUT_INDEX)],          // BRDF LUT
        linearSampler
    );

    iblAmbient *= lerp(shadowAmbientMin, 1.0, shadow);

    float3 finalColor = directLight + iblAmbient + emission;

    // 실내(Final): 어두운 영역에만 ambient색 fill을 더해 대비↓.
    // darkW가 밝은 픽셀(luma 높음)에서 0이라 밝은 데는 그대로, 어두운 데만 들어올림.
    if (overheadMode > 0.5)
    {
        float luma = dot(finalColor, float3(0.299, 0.587, 0.114));
        float darkW = saturate(1.0 - luma);
        finalColor += iblAmbient * darkW * (overheadAmbientBoost - 1.0);
    }

    float4 fog = fogTexture.Sample(linearSampler, input.uv);
    finalColor = finalColor * fog.a + fog.rgb;

    return float4(finalColor, 1.0);
}