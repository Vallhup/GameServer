#ifndef FOG_HLSLI
#define FOG_HLSLI

#include "ShaderResources.hlsli"

float3 ApplyFog(float3 color, float3 worldPos)
{
    // 거리 기반 안개
    float dist = length(cameraPosition - worldPos);
    float distFog = saturate((dist - fogStart) / fogRange);
    
    // 구간 안개: fogZoneStart ~ fogZoneEnd
    // 진입 보간
    float enterFog = saturate((worldPos.z - fogZoneStart) / fogZoneFade);
    
    // 퇴장 보간
    float exitFog = saturate((fogZoneEnd - worldPos.z) / fogZoneFade);
    
    // 둘 중 작은 값 (구간 내부에서만 1)
    float zoneFog = min(enterFog, exitFog);
    
    float fogAmount = distFog * zoneFog;
    
    return lerp(color, fogColor.rgb, fogAmount);
}

#endif