Texture2D colour_texture    : register(t0);
SamplerState border_sampler : register(s0);

struct pixel_shader_input
{
        float4 colour   : COLOR;
        float2 uv       : TEXCOORD;
};

float4 main(pixel_shader_input pi) : SV_TARGET
{
        return pi.colour * colour_texture.Sample(border_sampler, pi.uv);
}