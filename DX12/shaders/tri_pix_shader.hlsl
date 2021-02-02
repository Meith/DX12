Texture2D colour_texture                       : register(t0);
SamplerState border_sampler                    : register(s0);

struct pixel_shader_input
{
        float3 normal   : NORMAL;
        float2 texcoord : TEXCOORD;
        float4 tangent  : TANGENT;
};

float4 main(pixel_shader_input pi) : SV_TARGET
{
        return float4(pi.tangent);
}