struct pixel_shader_input
{
        float4 colour   : COLOR;
};

float4 main(pixel_shader_input pi) : SV_TARGET
{
        return pi.colour;
}