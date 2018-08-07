cbuffer per_frame_const_buffers : register(b0)
{
        matrix mvp;
}

struct vertex_input
{
        float3 position : POSITION;
        float3 colour   : COLOR;
};

struct vertex_output
{
        float4 colour   : COLOR;
        float4 position : SV_POSITION;
};

vertex_output main(vertex_input vi)
{
        vertex_output vo;
        vo.position = mul(mvp, float4(vi.position, 1.0f));
        vo.colour = float4(vi.colour, 1.0f);

        return vo;
}