cbuffer per_frame_const_buffers : register(b0)
{
    float2 window_bounds;
}

RaytracingAccelerationStructure rtx_acc_struct : register(t0);

RWTexture2D<float4> colour_tex : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID,
        uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
        float2 coord = float2(DTid.x, DTid.y);

        coord.x = (coord.x / window_bounds.x) * 2.0f - 1.0f;
        coord.y = (coord.y / window_bounds.y) * 2.0f - 1.0f;

        // Populate the ray
        RayDesc ray_desc;
        ray_desc.Origin = float3(coord.x, -coord.y, -0.01f); // start from top left screen coord.
        ray_desc.TMin = 0.0f;
        ray_desc.Direction = float3(0.0f, 0.0f, 1.0f); // shoot ray into screen
        ray_desc.TMax = 1.01f;

        // Instantiate ray query object.
        // Template parameter allows driver to generate a specialized
        // implementation.
        RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
                RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
                RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> ray_query;

        // Set up a trace.  No work is done yet.
        ray_query.TraceRayInline(
                rtx_acc_struct,
                RAY_FLAG_CULL_NON_OPAQUE |
                RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
                RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                0xFFu, // ?
                ray_desc);

        // Proceed() below is where behind-the-scenes traversal happens,
        // including the heaviest of any driver inlined code.
        // In this simplest of scenarios, Proceed() only needs
        // to be called once rather than a loop:
        // Based on the template specialization above,
        // traversal completion is guaranteed.
        ray_query.Proceed();

        // Examine and act on the result of the traversal.
        // Was a hit committed?
        if(ray_query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {

        }
        else {
                colour_tex[DTid.xy] = float4(0.57f, 0.8f, 0.95f, 1.0f);
        }
}