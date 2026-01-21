Texture2D<float> srcMip : register(t0);
RWTexture2D<float> dstMip : register(u0);
SamplerState pointSampler : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint dstWidth, dstHeight;
    dstMip.GetDimensions(dstWidth, dstHeight);
    
    if (DTid.x >= dstWidth || DTid.y >= dstHeight)
        return;
    
    float2 texelSize = 1.0f / float2(dstWidth * 2, dstHeight * 2);
    float2 uv = (float2(DTid.xy) * 2.0f + 0.5f) * texelSize;
    
    float d0 = srcMip.SampleLevel(pointSampler, uv, 0);
    float d1 = srcMip.SampleLevel(pointSampler, uv + float2(texelSize.x, 0), 0);
    float d2 = srcMip.SampleLevel(pointSampler, uv + float2(0, texelSize.y), 0);
    float d3 = srcMip.SampleLevel(pointSampler, uv + float2(texelSize.x, texelSize.y), 0);
    
    float maxDepth = max(max(d0, d1), max(d2, d3));
    dstMip[DTid.xy] = maxDepth;
}