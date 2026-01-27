#include "layout_defines.hlsl"
#include "entity.hlsl"

struct render_pass_data
{
    float4x4 view_proj;
};

struct sprite_instance
{
    uint entity_index;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

static const float2 k_corners[4] = {
    float2(-0.5, -0.5),
    float2(-0.5, 0.5),
    float2(0.5, -0.5),
    float2(0.5, 0.5)
};

static const float2 k_uvs[4] = {
    float2(0, 1),
    float2(0, 0),
    float2(1, 1),
    float2(1, 0)
};

vs_output VSMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    vs_output o;

    ConstantBuffer<render_pass_data> pass_params = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entities = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    StructuredBuffer<sprite_instance> instances = sfg_get_ssbo<sprite_instance>(sfg_rp_constant2);

    uint entity_index = instances[iid].entity_index;
    gpu_entity e = entities[entity_index];

    float2 corner = k_corners[vid];
    float4 world_pos = mul(e.model, float4(corner.x, corner.y, 0.0f, 1.0f));

    o.pos = mul(pass_params.view_proj, world_pos);
    o.uv = k_uvs[vid];
    return o;
}

struct material_data
{
    float4 base_color;
};

struct texture_data
{
    uint albedo_index;
};

SamplerState smp : static_sampler_linear;

float4 PSMain(vs_output IN) : SV_TARGET
{
    ConstantBuffer<material_data> mat = sfg_get_cbv<material_data>(sfg_mat_constant0);
    ConstantBuffer<texture_data> txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D albedo_txt = sfg_get_texture<Texture2D>(txt_data.albedo_index);
    float4 albedo = albedo_txt.Sample(smp, IN.uv);
    return albedo * mat.base_color;
}
