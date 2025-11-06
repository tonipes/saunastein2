#include "layout_defines.hlsl"
#include "entity.hlsl"
#include "bone.hlsl"

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
#ifdef USE_SKINNING
    float4 bone_weights : BLENDWEIGHT0;
    uint4  bone_indices : BLENDINDICES0;
#endif
};

struct vs_output
{
    float4 pos : SV_POSITION;
};

struct render_pass_data
{
    float4x4 view_proj;
};

vs_output VSMain(vs_input IN)
{
    vs_output OUT;
    render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float3 obj_norm;
    float4 obj_pos;
#ifdef USE_SKINNING
    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);
    float4 skinned_pos = float4(0,0,0,0);
    float3 skinned_norm = 0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index   = IN.bone_indices[i];
        float weight      = IN.bone_weights[i];
        float3x4 bone_mat = bone_buffer[bone_index].bone;
        skinned_pos += float4(mul(bone_mat, float4(IN.pos, 1.0f)) * weight, 1.0f);
        skinned_norm += mul(IN.normal, (float3x3)bone_mat) * weight;
    }
    obj_pos = skinned_pos;
    obj_norm = normalize(skinned_norm);
#else
    obj_pos = float4(IN.pos, 1.0f);
    obj_norm = normalize(IN.normal);
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    float3 world_norm = normalize(mul(entity.normal_matrix, float4(obj_norm, 1.0)).xyz);
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    return OUT;
}

float4 PSMain(vs_output IN) : SV_Target0
{
    return float4(1.0, 1.0, 1.0, 1.0);
}

