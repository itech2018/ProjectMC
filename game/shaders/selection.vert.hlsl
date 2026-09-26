cbuffer SelectionData : register(b0, space1)
{
    float4x4 viewProjection;
    float4 blockOrigin;
};

struct VSOutput { float4 position : SV_Position; };

VSOutput main(uint vertexId : SV_VertexID)
{
    static const uint2 edges[12] = {
        uint2(0,1),uint2(1,3),uint2(3,2),uint2(2,0),
        uint2(4,5),uint2(5,7),uint2(7,6),uint2(6,4),
        uint2(0,4),uint2(1,5),uint2(2,6),uint2(3,7)
    };
    uint edge=vertexId/2;
    uint endpoint=vertexId%2;
    uint corner=edges[edge][endpoint];
    const float e=0.003;
    float3 p=float3(
        (corner&1)?1.0+e:-e,
        (corner&2)?1.0+e:-e,
        (corner&4)?1.0+e:-e
    )+blockOrigin.xyz;
    VSOutput o;
    o.position=mul(viewProjection,float4(p,1));
    return o;
}
