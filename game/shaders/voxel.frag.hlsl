Texture2D atlasTexture : register(t0, space2);
SamplerState atlasSampler : register(s0, space2);

struct PSInput {
 float4 position : SV_Position;
 float2 tileUv : TEXCOORD0;
 float shade : TEXCOORD1;
 float opacity : TEXCOORD2;
 float4 atlasRegion : TEXCOORD3;
};
float4 main(PSInput input) : SV_Target0 {
 float2 local=frac(input.tileUv);
 // Keep exact integer edges on the far side of a merged quad inside the tile.
 if(input.tileUv.x>0 && abs(local.x)<0.00001) local.x=0.9999;
 if(input.tileUv.y>0 && abs(local.y)<0.00001) local.y=0.9999;
 float2 uv=lerp(input.atlasRegion.xy,input.atlasRegion.zw,local);
 float4 texel=atlasTexture.Sample(atlasSampler,uv);
 texel.rgb*=input.shade;
 texel.a*=input.opacity;
 return texel;
}
