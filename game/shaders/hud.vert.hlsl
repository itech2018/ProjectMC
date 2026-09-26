struct VSOutput { float4 position : SV_Position; float4 color : TEXCOORD0; };

VSOutput main(uint vertexId : SV_VertexID)
{
    static const float2 p[12] = {
        float2(-0.010, -0.0015), float2( 0.010, -0.0015), float2( 0.010,  0.0015),
        float2(-0.010, -0.0015), float2( 0.010,  0.0015), float2(-0.010,  0.0015),
        float2(-0.0015, -0.016), float2( 0.0015, -0.016), float2( 0.0015,  0.016),
        float2(-0.0015, -0.016), float2( 0.0015,  0.016), float2(-0.0015,  0.016)
    };
    VSOutput o;
    o.position = float4(p[vertexId], 0.0, 1.0);
    o.color = float4(1.0, 1.0, 1.0, 1.0);
    return o;
}
