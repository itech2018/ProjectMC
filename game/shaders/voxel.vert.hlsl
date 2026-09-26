cbuffer Camera : register(b0, space1) { float4x4 viewProjection; };

struct VSInput {
 float3 position : TEXCOORD0;
 float2 uv : TEXCOORD1;
 float shade : TEXCOORD2;
 float opacity : TEXCOORD3;
 float4 atlasRegion : TEXCOORD4;
};
struct VSOutput {
 float4 position : SV_Position;
 float2 tileUv : TEXCOORD0;
 float shade : TEXCOORD1;
 float opacity : TEXCOORD2;
 float4 atlasRegion : TEXCOORD3;
};
VSOutput main(VSInput input) {
 VSOutput o;
 o.position=mul(viewProjection,float4(input.position,1));
 o.tileUv=input.uv;
 o.shade=input.shade;
 o.opacity=input.opacity;
 o.atlasRegion=input.atlasRegion;
 return o;
}
