cbuffer LightCB : register(b3)
{
    int lightCount;
    float3 padding;
    
    struct LightData
    {
        float3 position;
        float range;
        float3 color;
        float intensity;
        int type;
        float3 lightPadding;
    } lights[50];
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gBufferPosition : register(t4);
Texture2D gBufferNormal : register(t5); 
Texture2D gBufferAlbedo : register(t6); 
SamplerState pointSampler : register(s0);

float4 PSMain(PS_IN input) : SV_Target
{
    float4 position = gBufferPosition.Sample(pointSampler, input.uv);
    float4 normal = gBufferNormal.Sample(pointSampler, input.uv);
    float4 albedo = gBufferAlbedo.Sample(pointSampler, input.uv);
    
    if (position.w == 0.0f)
    {
        return albedo;
    }
    
    float3 worldPos = position.xyz;
    float3 worldNormal = normalize(normal.xyz);
    float3 finalColor = float3(0, 0, 0);
    
    for (int i = 0; i < lightCount; ++i)
    {
        float3 lightContribution = float3(0, 0, 0);
        
        if (lights[i].type == 0) 
        {
            float3 lightDir = normalize(-lights[i].position); 
            float NdotL = max(0.0, dot(worldNormal, lightDir));
            lightContribution = albedo.rgb * lights[i].color * lights[i].intensity * NdotL;
        }
        else if (lights[i].type == 1) 
        {
            float3 lightVec = lights[i].position - worldPos;
            float distance = length(lightVec);
            
            if (distance < lights[i].range)
            {
                float3 lightDir = normalize(lightVec);
                float NdotL = max(0.0, dot(worldNormal, lightDir));
                
                float attenuation = 1.0 - (distance / lights[i].range);
                attenuation = attenuation * attenuation; 
                
                lightContribution = albedo.rgb * lights[i].color * lights[i].intensity * NdotL * attenuation;
            }
        }
        
        finalColor += lightContribution;
    }
    
    finalColor += albedo.rgb * 0.1f;
    
    return float4(finalColor, albedo.a);
}