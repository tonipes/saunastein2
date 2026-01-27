#include "layout_defines.hlsl"
#include "particle/particles.hlsl"
#include "packing_utils.hlsl"

struct vs_output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD4;
};

static const float2 k_corners[4] = {
    float2(-1, -1),
    float2(-1, +1),
    float2(+1, -1),
    float2(+1, +1)
};

static const float2 k_uvs[4] = {
    float2(0, 1),
    float2(0, 0),
    float2(1, 1),
    float2(1, 0)
};

struct material_data
{
    float4 base_color;
    float2 stretch_params;
};

vs_output VSMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    vs_output o;

    StructuredBuffer<particle_instance_data> instance_data = sfg_get_ssbo<particle_instance_data>(sfg_rp_constant1);
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    ConstantBuffer<material_data> mat = sfg_get_cbv<material_data>(sfg_mat_constant0);

    particle_instance_data p = instance_data[iid];

    float3 center = p.pos_rot_size.xyz;
    float2 rot_size = unpack_rot_size((uint)p.pos_rot_size.w);
    float4 color = unpack_rgba8_unorm(p.color);
    float3 vel = p.velocity.xyz;

    float speed = length(vel);
    float3 forward = speed > 0.0001f ? (vel / speed) : normalize(pass_params.cam_dir.xyz);
    float3 cam_dir = normalize(pass_params.cam_dir.xyz);

    float3 right = cross(forward, cam_dir);
    float right_len = length(right);
    if (right_len < 0.001f)
        right = cross(forward, float3(0, 1, 0));
    right = normalize(right);

    float3 up = normalize(cross(right, forward));

    float half_length = max(rot_size.y, speed * mat.stretch_params.x);
    float half_width = rot_size.y * mat.stretch_params.y;

    float2 corner = k_corners[vid];

    float3 world_pos =
        center +
        right * (corner.x * half_width) +
        forward * (corner.y * half_length);

    o.pos = mul(pass_params.view_proj, float4(world_pos, 1.0f));
    o.uv = k_uvs[vid];
    o.color = color;
    return o;
}

float4 PSMain(vs_output IN) : SV_TARGET
{
    ConstantBuffer<material_data> mat = sfg_get_cbv<material_data>(sfg_mat_constant0);
    float taper = saturate(1.0f - abs(IN.uv.y - 0.5f) * 2.0f);
    float4 col = IN.color * mat.base_color;
    col.w *= taper;
    return col;
}
