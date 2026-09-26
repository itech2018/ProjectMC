// ProjectMC voxel fragment shader.

Texture2D atlasTexture : register(t0, space2);
SamplerState atlasSampler : register(s0, space2);

struct PSInput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float  shade    : TEXCOORD1;
    float  opacity  : TEXCOORD2;
};

float4 main(PSInput input) : SV_Target0
{
    float4 texel = atlasTexture.Sample(atlasSampler, input.uv);
    texel.rgb *= input.shade;
    texel.a *= input.opacity;
    return texel;
}
