#include "../layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

cbuffer render_pass : render_pass_ubo0
{
    float3 ambient_color;
    uint point_lights_count;
    uint spot_lights_count;
    uint dir_lights_count;
    uint pad[2];
};

struct gpu_point_light
{
    uint entity_index;
    float3 color;
    float range;
    float intensity;
    float2 pad;
};

struct gpu_spot_light
{
    uint entity_index;
    float3 color;
    float range;
    float intensity;
    float inner_cone;
    float outer_cone;
};

struct gpu_dir_light
{
    uint entity_index;
    float3 color;
    float intensity;
    float3 pad;
};

struct gpu_entity
{
    float3x4 model;
    float3x3 normal_matrix;
    float3 pad;
};

StructuredBuffer<gpu_entity> entity_buffer : render_pass_ssbo0;
StructuredBuffer<gpu_point_light> point_light_buffer : render_pass_ssbo1;
StructuredBuffer<gpu_spot_light> spot_light_buffer : render_pass_ssbo2;
StructuredBuffer<gpu_dir_light> dir_light_buffer : render_pass_ssbo3;

Texture2D tex_gbuffer_color : render_pass_texture0;
Texture2D tex_gbuffer_normal: render_pass_texture1;
Texture2D tex_gbuffer_orm : render_pass_texture2;
Texture2D tex_gbuffer_emissive : render_pass_texture3;
SamplerState smp_linear : static_sampler_linear;

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(uint vertexID : SV_VertexID)
{
    vs_output OUT;
	
    float2 pos = float2(
        (vertexID == 2) ? 3.0 : -1.0,
        (vertexID == 1) ? -3.0 : 1.0
    );
	
    OUT.pos = float4(pos, 0.0f, 1.0f);
    OUT.uv = 0.5f * (pos + 1.0f);
    OUT.uv.y = 1.0f - OUT.uv.y;
    return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
    float4 out_color = tex_gbuffer_color.SampleLevel(smp_linear, IN.uv, 0) * float4(ambient_color, 1);
    return out_color ;
}

