#include "layout_defines.hlsl"
#include "packing_utils.hlsl"
#include "entity.hlsl"
#include "normal.hlsl"
#include "bone.hlsl"
#include "render_pass_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------


struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD4;
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_forward rp_data = sfg_get_cbv<render_pass_data_forward>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;

    obj_pos = float4(IN.pos, 1.0f);

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.uv = float2(world_pos.x, world_pos.z) * 0.1;

    return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------


struct material_data
{
    float4 base_color_factor;
};

struct texture_data
{
    uint gpu_index_albedo;
};

float2 hash2(float2 p)
{
    float2 k = float2(127.1f, 311.7f);
    float2 k2 = float2(269.5f, 183.3f);
    float2 s = float2(dot(p, k), dot(p, k2));
    return frac(sin(s) * 43758.5453f);
}

float2 rot2(float2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

#ifdef WRITE_ID

uint PSMain(vs_output IN) : SV_TARGET
{
    return sfg_object_constant2;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_forward params = sfg_get_cbv<render_pass_data_forward>(sfg_rp_constant0);

    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float t = sfg_global_elapsed;
    float2 uv = IN.uv;
    uv += float2(t * 0.05f, t * 0.03f);
    uv += sin(float2(uv.y, uv.x) * 10.0f + t * 1.4f) * 0.025f;
    uv += sin(float2(uv.y, uv.x) * 22.0f - t * 2.3f) * 0.012f;

    float2 tile = floor(uv);
    float2 local_uv = frac(uv);
    float2 rnd = hash2(tile);
    float angle = (rnd.x * 2.0f - 1.0f) * 0.35f;
    float2 uv0 = rot2(local_uv - 0.5f, angle) + 0.5f + (rnd - 0.5f) * 0.12f;

    float2 tile_b = tile + float2(1.0f, 1.0f);
    float2 rnd_b = hash2(tile_b);
    float angle_b = (rnd_b.x * 2.0f - 1.0f) * 0.35f;
    float2 uv1 = rot2(local_uv - 0.5f, angle_b) + 0.5f + (rnd_b - 0.5f) * 0.12f;

    float blend = smoothstep(0.35f, 0.65f, rnd.y);
    float4 albedo_tex = lerp(
        tex_albedo.Sample(sampler_default, uv0),
        tex_albedo.Sample(sampler_default, uv1),
        blend
    );
    float4 albedo = albedo_tex * mat_data.base_color_factor * params.ambient ;
    albedo.a = 0.8f;

    return albedo;
    // return float4(tex_albedo, 1.0);
    // return float4(1.0, 1.0, 1.0, 0.5);
}

#endif

