
struct PS_IN
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

Texture2D depthTexture : register(t4);
Texture2D normalTexture : register(t5);

SamplerState pointSampler : register(s0);

float4 PSMain(PS_IN input) : SV_Target
{
    float depth = depthTexture.Sample(pointSampler, input.uv).r;
    float3 normal = normalTexture.Sample(pointSampler, input.uv).xyz;
    
    if (depth >= 1.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    float ao = 0.0f;
    float radius = 0.01f;   // »ùÇÃ¸µ ¹İ°æ
    
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y < 1; y++)
        {
            if (x == 0 && y == 0)
                continue;
            
            // TODO (SSAO »ùÇÃ¸µ)
        }

    }
    
    return float4(ao, ao, ao, 1.0f);
}