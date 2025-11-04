#include "layout_defines.hlsl"
#include "packing_utils.hlsl"
#include "entity.hlsl"
#include "normal.hlsl"
#include "bone.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

#ifdef USE_SKINNING

struct vs_input
{
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
    float4 bone_weights : BLENDWEIGHT0;
    uint4  bone_indices : BLENDINDICES0;
};

#else

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

#endif

struct vs_output
{
    float4 pos : SV_POSITION;
    float3 world_norm : TEXCOORD0;
    float3 world_tan : TEXCOORD1;
    float3 world_bit : TEXCOORD2;
    float2 uv : TEXCOORD4;
};

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

struct render_pass_data
{
    float4x4 view_proj;
    float4 ambient;
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;
    float3 obj_norm;
    float3 obj_tan;

#ifdef USE_SKINNING

    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);

    // skinning in object space
    float4 skinned_pos    = float4(0, 0, 0, 0);
    float3 skinned_normal = 0;
    float3 skinned_tan    = 0;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index   = IN.bone_indices[i];
        float weight      = IN.bone_weights[i];
        float3x4 bone_mat = bone_buffer[bone_index].bone;

        skinned_pos    += float4(mul(bone_mat, float4(IN.pos, 1.0f)) * weight, 1.0f);
        skinned_normal += mul(IN.normal, (float3x3)bone_mat) * weight;
        skinned_tan    += mul(IN.tangent.xyz, (float3x3)bone_mat) * weight;
    }

    obj_pos  = skinned_pos;
    obj_norm = skinned_normal;
    obj_tan  = skinned_tan;
#else
    obj_pos = float4(IN.pos, 1.0f);
    obj_norm = IN.normal;
    obj_tan = IN.tangent.xyz;
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    float3 N = normalize(mul(entity.normal_matrix, float4(obj_norm, 1.0)).xyz);
    float3 T = normalize(mul(entity.normal_matrix, float4(obj_tan, 1.0)).xyz);

    // gram-schmidt ensure orthogonality.
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T)) * IN.tangent.w;

    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.world_norm = N;
    OUT.world_tan = T;
    OUT.world_bit = B;
    OUT.uv = IN.uv;

    return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------


struct material_data
{
    float4 base_color_factor;
    uint2 albedo_tiling_offset;
};

struct texture_data
{
    uint gpu_index_albedo;
    uint gpu_index_sampler;
};

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data params = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(txt_data.gpu_index_sampler);

    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float2 albedo_uv = IN.uv * albedo_tiling + albedo_offset;

    // --- Base color ---
    float4 albedo_tex = tex_albedo.Sample(sampler_default, albedo_uv);
    float4 albedo = albedo_tex * mat_data.base_color_factor * params.ambient ;

    return albedo;
}

