struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    
    float2 positions[6] =
    {
        float2(-1, -1),
        float2(-1, 1), 
        float2(1, -1), 
        float2(1, -1), 
        float2(-1, 1), 
        float2(1, 1) 
    };
    
    float2 uvs[6] =
    {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 1),
        float2(0, 0),
        float2(1, 0) 
    };
    
    output.pos = float4(positions[vertexID], 0, 1);
    output.uv = uvs[vertexID];
    
    return output;
}