cbuffer HudData : register(b0, space1)
{
    float4 rects[12];   // xy=center, zw=half size in NDC
    float4 colors[12];
    uint rectCount;
    float3 padding;
};

struct VSOutput { float4 position : SV_Position; float4 color : TEXCOORD0; };

VSOutput main(uint vertexId : SV_VertexID)
{
    static const float2 corners[6] = {
        float2(-1,-1), float2(1,-1), float2(1,1),
        float2(-1,-1), float2(1,1), float2(-1,1)
    };
    uint rectIndex=vertexId/6;
    uint cornerIndex=vertexId%6;
    VSOutput o;
    float4 r=rects[rectIndex];
    o.position=float4(r.xy+corners[cornerIndex]*r.zw,0,1);
    o.color=colors[rectIndex];
    return o;
}
