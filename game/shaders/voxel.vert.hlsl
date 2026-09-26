// ProjectMC voxel vertex shader.
// Source is compiled to the backend-specific format during shader builds.

cbuffer Camera : register(b0, space1)
{
    float4x4 viewProjection;
};

struct VSInput
{
    float3 position : TEXCOORD0;
    float2 uv       : TEXCOORD1;
    float  shade    : TEXCOORD2;
    float  opacity  : TEXCOORD3;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float  shade    : TEXCOORD1;
    float  opacity  : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(viewProjection, float4(input.position, 1.0));
    output.uv = input.uv;
    output.shade = input.shade;
    output.opacity = input.opacity;
    return output;
}
