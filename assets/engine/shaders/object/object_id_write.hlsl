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

    float4 obj_pos;
#ifdef USE_SKINNING
    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);
    float4 skinned_pos = float4(0,0,0,0);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index   = IN.bone_indices[i];
        float weight      = IN.bone_weights[i];
        float3x4 bone_mat = bone_buffer[bone_index].bone;
        skinned_pos += float4(mul(bone_mat, float4(IN.pos, 1.0f)) * weight, 1.0f);
    }
    obj_pos = skinned_pos;
#else
    obj_pos = float4(IN.pos, 1.0f);
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    return OUT;
}

uint PSMain(vs_output IN) : SV_Target0
{
    // Write actual entity world_id bound at object_constant1
    return sfg_object_constant1;
}

