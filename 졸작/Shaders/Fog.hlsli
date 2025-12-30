#ifndef FOG_HLSLI
#define FOG_HLSLI

#include "ShaderResources.hlsli"

float3 ApplyFog(float3 color, float3 worldPos)
{
    float dist = length(cameraPosition - worldPos);
    float distFog = saturate((dist - fogStart) / fogRange);
    
    float enterFog = saturate((worldPos.z - fogZoneStart) / fogZoneFade);
    
    float exitFog = saturate((fogZoneEnd - worldPos.z) / fogZoneFade);
    
    float zoneFog = min(enterFog, exitFog);
    
    float fogAmount = distFog * zoneFog;
    
    return lerp(color, fogColor.rgb, fogAmount);
}

#endif