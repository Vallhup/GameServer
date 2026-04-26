#ifndef CLUSTERCULLCS_HLSLI
#define CLUSTERCULLCS_HLSLI

#include "ShaderResources.hlsli"
#include "Constants.hlsli"

groupshared uint   sLocalCount;
groupshared uint   sLocalIndices[MAX_LIGHTS_PER_CLUSTER];
groupshared float3 sClusterMin;
groupshared float3 sClusterMax;
groupshared uint   sGlobalOffset;
groupshared uint   sFinalCount;

float3 NdcToWorld(float3 ndc)
{
    float4 clipPos = float4(ndc, 1.0);
    float4 wp = mul(clipPos, invViewProj);
    return wp.xyz / wp.w;
}

bool SphereAABBIntersect(float3 c, float r, float3 mn, float3 mx)
{
    float3 closest = clamp(c, mn, mx);
    float3 d = c - closest;
    return dot(d, d) <= r * r;
}

[numthreads(THREADS_PER_GROUP, 1, 1)]
void CSMain(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint clusterX = groupID.x;
    uint clusterY = groupID.y;
    uint clusterZ = groupID.z;
    uint clusterIdx = clusterZ * clusterGridDims.x * clusterGridDims.y
                    + clusterY * clusterGridDims.x
                    + clusterX;

    if (groupIndex == 0)
    {
        sLocalCount   = 0;
        sGlobalOffset = 0;
        sFinalCount   = 0;

        float gx = (float)clusterGridDims.x;
        float gy = (float)clusterGridDims.y;
        float gz = (float)clusterGridDims.z;

        // NDC x: 0=left(-1), gridX=right(+1)
        float ndcMinX = ((float)clusterX       / gx) * 2.0 - 1.0;
        float ndcMaxX = ((float)(clusterX + 1) / gx) * 2.0 - 1.0;

        // NDC y: 0=top(+1), gridY=bottom(-1) — PS uv.y 0=top 와 일관
        float ndcMaxY = 1.0 - 2.0 * ((float)clusterY       / gy);
        float ndcMinY = 1.0 - 2.0 * ((float)(clusterY + 1) / gy);

        // exponential view-Z slicing
        float zRatio    = clusterZFar / clusterZNear;
        float viewZNear = clusterZNear * pow(zRatio, (float)clusterZ       / gz);
        float viewZFar  = clusterZNear * pow(zRatio, (float)(clusterZ + 1) / gz);

        // D3D12 standard projection (LH, NDC z 0~1) : ndcZ = a + b/viewZ
        float a = clusterZFar / (clusterZFar - clusterZNear);
        float b = -clusterZFar * clusterZNear / (clusterZFar - clusterZNear);
        float ndcZNear = a + b / viewZNear;
        float ndcZFar  = a + b / viewZFar;

        float3 mn = float3( 1e30,  1e30,  1e30);
        float3 mx = float3(-1e30, -1e30, -1e30);

        [unroll]
        for (uint i = 0; i < 8; ++i)
        {
            float xN = (i & 1) ? ndcMaxX : ndcMinX;
            float yN = (i & 2) ? ndcMaxY : ndcMinY;
            float zN = (i & 4) ? ndcZFar : ndcZNear;
            float3 wp = NdcToWorld(float3(xN, yN, zN));
            mn = min(mn, wp);
            mx = max(mx, wp);
        }

        sClusterMin = mn;
        sClusterMax = mx;
    }
    GroupMemoryBarrierWithGroupSync();

    // 각 thread가 LIGHTS_PER_THREAD 개 라이트 sweep — 64 * 4 = 256 (LightManager::MAX_LIGHTS 와 동일)
    uint activeCount = (uint)lightCount;

    [unroll]
    for (uint i = 0; i < LIGHTS_PER_THREAD; ++i)
    {
        uint lightId = groupIndex + i * THREADS_PER_GROUP;
        if (lightId >= activeCount)
            continue;

        LightData L = lights[lightId];
        bool passed = false;

        if (L.type == 0)
        {
            // directional : 활성 sun 이면 모든 클러스터 통과
            passed = (L.intensity > 0.0);
        }
        else
        {
            // point : sphere - AABB 교차
            if (L.intensity > 0.0 && L.range > 0.0)
                passed = SphereAABBIntersect(L.position, L.range, sClusterMin, sClusterMax);
        }

        if (passed)
        {
            uint slot;
            InterlockedAdd(sLocalCount, 1, slot);
            if (slot < MAX_LIGHTS_PER_CLUSTER)
                sLocalIndices[slot] = lightId;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // thread 0 이 globalCounter atomic 으로 offset 확보 + lightGrid 기록
    if (groupIndex == 0)
    {
        uint clamped = min(sLocalCount, MAX_LIGHTS_PER_CLUSTER);
        uint globalOffset;
        InterlockedAdd(clusterCounterRW[0], clamped, globalOffset);

        sGlobalOffset = globalOffset;
        sFinalCount   = clamped;

        clusterLightGridRW[clusterIdx] = uint2(globalOffset, clamped);
    }
    GroupMemoryBarrierWithGroupSync();

    // 각 thread가 indices 평탄 배열로 복사
    for (uint k = groupIndex; k < sFinalCount; k += THREADS_PER_GROUP)
    {
        clusterLightIndicesRW[sGlobalOffset + k] = sLocalIndices[k];
    }
}

#endif
