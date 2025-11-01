#include "layout_defines.hlsl"
#include "entity.hlsl"
#include "light.hlsl"
#include "pbr.hlsl"
#include "depth.hlsl"
#include "normal.hlsl"

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

struct render_pass_data
{
    float4x4 inv_view_proj;
    float4 ambient_color_plights_count;
    float4 view_pos_slights_count;
    uint dir_lights_count;
    uint cascade_levels_gpu_index;
    uint cascades_count;
    float near_plane;
    float far_plane;
    float pad[3];
};

SamplerState smp_linear : static_sampler_linear;
SamplerState smp_nearest: static_sampler_nearest;
SamplerComparisonState smp_shadow : static_sampler_shadow_2d;

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(uint vertexID : SV_VertexID)
{
    vs_output OUT;
	
    float2 pos;
    if (vertexID == 0) pos = float2(-1.0, -1.0);
    else if (vertexID == 1) pos = float2(-1.0,  3.0);
    else                  pos = float2( 3.0, -1.0);

    OUT.pos = float4(pos, 0.0, 1.0);
    OUT.uv  = pos * float2(0.5, -0.5) + 0.5;
    return OUT;
}

// Very small epsilon guard
static bool is_background(float device_depth)
{
    // Reversed-Z: far = 0, near = 1. Treat ~0 as background
    return device_depth <= 1e-6;
}

float2 ndc_to_uv(float2 ndc_xy) { return ndc_xy * 0.5f + 0.5f; }

static const float g_depth_bias_base   = 0.0008f;   // base receiver bias (world->light clip->depth)
static const float g_normal_bias_scale = 0.1f;      // scales with slope (bigger -> fewer acne, more peter-panning)
static const int   g_pcf_radius        = 1;         // 0: 1 tap, 1: 3x3, 2: 5x5
static const float g_cascade_blend     = 0.5f;      // optional cross-fade width in normalized depth (0 = off)

// Normal/slope-based bias term (NoL-aware).
float slope_bias(float NoL)
{
    float s = sqrt(saturate(1.0f - NoL * NoL));
    return g_normal_bias_scale * (s / max(NoL, 1e-3f));
}

bool outside(float2 uv) { return any(uv < 0.0f) || any(uv > 1.0f); }

float pcf_cascade(int radius, Texture2DArray shadow_map, SamplerComparisonState smp, float2 uv, int slice, float compare_depth, float2 texel)
{
     // PCF kernel (square)
    int r = radius;                 // 1 => 3x3, 2 => 5x5
    float taps = 0.0f;
    float accum = 0.0f;

    [unroll]
    for (int dy = -r; dy <= +r; ++dy)
    {
        [unroll]
        for (int dx = -r; dx <= +r; ++dx)
        {
            float2 offs = float2(dx, dy) * texel;
            accum += shadow_map.SampleCmpLevelZero(smp_shadow, float3(uv + offs, slice), compare_depth);
            taps += 1.0f;
        }
    }
    return accum / max(taps, 1.0f);
}

float pcf(int radius, Texture2D shadow_map, SamplerComparisonState smp, float2 uv, float compare_depth, float2 texel)
{
     // PCF kernel (square)
    int r = radius;                 // 1 => 3x3, 2 => 5x5
    float taps = 0.0f;
    float accum = 0.0f;

    [unroll]
    for (int dy = -r; dy <= +r; ++dy)
    {
        [unroll]
        for (int dx = -r; dx <= +r; ++dx)
        {
            float2 offs = float2(dx, dy) * texel;
            accum += shadow_map.SampleCmpLevelZero(smp_shadow, float2(uv + offs), compare_depth);
            taps += 1.0f;
        }
    }
    return accum / max(taps, 1.0f);
}

float sample_cascade_shadow(
    Texture2DArray shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3         world_pos,
    float3         N,
    float3         L,
    int            slice,
    float2         shadow_resolution, float texel_world)
{
    // Transform world position into light clip space
    float4 clip = mul(light_space_matrix, float4(world_pos, 1.0f));
    if (clip.w <= 0.0f) return 1.0f;
    float2 uv = ndc_to_uv(clip.xy);
    if (outside(uv)) return 1.0f;
    uv.y = 1.0 - uv.y;

    float2 texel = 1.0f / shadow_resolution;

    // we try to trust rasterizer bias, if not below
    // float NoL = saturate(dot(N, normalize(L)));
    // float receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y); // or texel_world * 0.00001

    float compare_depth = clip.z; 

     // Single tap (fast path)
    if (g_pcf_radius == 0)
    {
        float val = shadow_map.SampleCmpLevelZero(smp, float3(uv, slice), compare_depth);
        return val;
    }

    return pcf_cascade(g_pcf_radius, shadow_map, smp, uv, slice, compare_depth, texel);
}

float sample_shadow_cone(Texture2D shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3         world_pos,
    float3         N,
    float3         L,
    float3 light_forward,
    float2         shadow_resolution,
    float cos_outer
    )
{
    float cos_theta = dot(normalize(-L), normalize(light_forward));
    if (cos_theta < cos_outer) return 1.0f;

    float4 clip = mul(light_space_matrix, float4(world_pos, 1.0f));

    // Guard against points behind the light camera
    if (clip.w <= 0.0f) return 1.0f;

    float3 ndc = clip.xyz;
    ndc /= clip.w;

    float2 uv = ndc_to_uv(ndc.xy);
    if (outside(uv)) return 1.0f;
    uv.y = 1.0 - uv.y;

    float NoL = saturate(dot(N, normalize(L)));
    float2 texel = 1.0f / shadow_resolution;
    float receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y);

    float compare_depth = ndc.z - receiver_bias * 0.1;

    if (g_pcf_radius == 0)
    {
        float val = shadow_map.SampleCmpLevelZero(smp, uv, compare_depth);
        return val;
    }

    return pcf(g_pcf_radius, shadow_map, smp, uv,  compare_depth, texel);
}

float depth01_from_eyeZ(float z_eye, float nearZ, float farZ)
{
    float a =  farZ / (farZ - nearZ);
    float b = -nearZ * farZ / (farZ - nearZ);
    return a + b / z_eye; 
}

float sample_shadow_cube(TextureCube shadow_map,
    SamplerComparisonState smp,
    float4x4       light_space_matrix,
    float3 light_pos,
    float3         world_pos,
    float3         N,
    float3         L,
    float2         shadow_resolution,
    float near_z, float far_z
    )
{

    float3 R   = world_pos - light_pos;
    float3 dir = normalize(R);
    dir.z = -dir.z;
    
    // Eye-space z for the face the cube lookup will choose
    float z_eye = max(abs(R.x), max(abs(R.y), abs(R.z)));

    // Depth in 0..1 matching what was written to the cube depth faces
    float depth01 = depth01_from_eyeZ(z_eye, near_z, far_z);

    float2 texel = 1.0 / shadow_resolution;
    float  NoL   = saturate(dot(N, normalize(L)));
    float  receiver_bias = g_depth_bias_base + slope_bias(NoL) * max(texel.x, texel.y);

    float  cmp = depth01 - receiver_bias * 0.1;
    
    // 1-tap compare; PCF for cubes needs angular offsets (see note below)
    return shadow_map.SampleCmpLevelZero(smp, dir, cmp);
}

float sample_cascade_shadow_blend(
    Texture2DArray shadow_map,
    SamplerComparisonState smp_shadow,
    float4x4       light_space_matrix_curr,
    float4x4       light_space_matrix_next,
    float3         world_pos,
    float3         N,
    float3         L,
    int            slice_curr,
    int            slice_next,
    float2         shadow_resolution,
    float          depth_linear,           // eye-linear 
    float          split_curr,             // end depth of current cascade
    float          blend_width, float texel_world)    
{
    if (blend_width <= 0.0f || slice_next < 0)
    {
        return sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world);
    }

    float a = saturate( (split_curr - depth_linear) / max(blend_width, 1e-6f) );
    // a=1 => fully current cascade, a=0 => transition to next
    float s0 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_curr, world_pos, N, L, slice_curr, shadow_resolution, texel_world);
    float s1 = sample_cascade_shadow(shadow_map, smp_shadow, light_space_matrix_next, world_pos, N, L, slice_next, shadow_resolution, texel_world);
    
    return lerp(s1, s0, a);
}

//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data rp_data = sfg_get_rp_cbv<render_pass_data>();
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant0);
    StructuredBuffer<gpu_point_light> point_light_buffer = sfg_get_ssbo<gpu_point_light>(sfg_rp_constant1);
    StructuredBuffer<gpu_spot_light> spot_light_buffer = sfg_get_ssbo<gpu_spot_light>(sfg_rp_constant2);
    StructuredBuffer<gpu_dir_light> dir_light_buffer = sfg_get_ssbo<gpu_dir_light>(sfg_rp_constant3);
    StructuredBuffer<gpu_shadow_data> shadow_data_buffer = sfg_get_ssbo<gpu_shadow_data>(sfg_rp_constant4);
    StructuredBuffer<float> float_buffer = sfg_get_ssbo<float>(sfg_rp_constant5);
    Texture2D tex_gbuffer_color = sfg_get_texture2D(sfg_rp_constant6);
    Texture2D tex_gbuffer_normal = sfg_get_texture2D(sfg_rp_constant7);
    Texture2D tex_gbuffer_orm = sfg_get_texture2D(sfg_rp_constant8);
    Texture2D tex_gbuffer_emissive = sfg_get_texture2D(sfg_rp_constant9);
    Texture2D tex_gbuffer_depth = sfg_get_texture2D(sfg_rp_constant10);

     // pixel for .Load()
    int2 ip = int2(IN.pos.xy);

    // fetch depth
    float device_depth = tex_gbuffer_depth.Load(int3(ip, 0)).r;

    // background early-out (reversed-Z)
    if (is_background(device_depth))
        return float4(0,0,0,1);

    // reconstruct world position
    float2 uv = IN.uv;
    float3 world_pos = reconstruct_world_position(uv, device_depth, rp_data.inv_view_proj);
    float3 V = normalize(rp_data.view_pos_slights_count.xyz - world_pos);

    // decode gbuffer
    float4 albedo_data   = tex_gbuffer_color.SampleLevel(smp_nearest, uv, 0);      // sRGB RT was linearized when written; this is now linear
    float4 normal_data = tex_gbuffer_normal.Load(int3(ip, 0));          // RGB10A2_UNORM -> oct encode
    float4 orm_data      = tex_gbuffer_orm.Load(int3(ip, 0));      // [ao, rough, metal, _]
    float4 emissive_data = tex_gbuffer_emissive.Load(int3(ip, 0));    // linear
    float3 albedo = albedo_data.xyz;
    float3 emissive = emissive_data.xyz;
    float  ao        = saturate(orm_data.r);
    float  roughness = saturate(orm_data.g);
    float  metallic  = saturate(orm_data.b);
    float3 N = oct_decode(normal_data.xy);

    float3 ambientColor = rp_data.ambient_color_plights_count.xyz;
    float3 lighting = ambientColor * albedo * ao;

    // fetch light prep
    uint point_light_count = uint(rp_data.ambient_color_plights_count.w);
    uint spot_light_count = uint(rp_data.view_pos_slights_count.w);
    uint dir_light_count = rp_data.dir_lights_count;

    [loop]
    for(uint i = 0; i < point_light_count; i++)
    {
        gpu_point_light light = point_light_buffer[i];
        gpu_entity entity = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = entity.position.xyz;
        float intensity = light.intensity_range.x;
        float range = light.intensity_range.y;

        float3 Lvec = light_pos - world_pos;
        float  d  = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;
        
        float  att = attenuation(range, d);
        float3 radiance  = light_col * (intensity * att);

        float shadow_vis = 1.0;
        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            TextureCube shadow_map = sfg_get_texturecube(shadow_map_index);
            gpu_shadow_data sd = shadow_data_buffer[shadow_data_index];
            float4x4 light_space = sd.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;

            shadow_vis = sample_shadow_cube(shadow_map, smp_shadow, light_space, light_pos, world_pos, N, L, shadow_resolution, rp_data.near_plane,light.far_plane);
        }

        lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);
    }

    [loop]
    for(uint i = 0; i < spot_light_count; i++)
    {
        gpu_spot_light light = spot_light_buffer[i];                    // FIXED
        gpu_entity e = entity_buffer[uint(light.color_entity_index.w)];
        float3 entity_forward = e.forward.xyz;
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = e.position.xyz;
        float  intensity = light.intensity_range_inner_outer.x;
        float  range     = light.intensity_range_inner_outer.y;
        float  cosInner  = saturate(light.intensity_range_inner_outer.z);
        float  cos_outer  = saturate(light.intensity_range_inner_outer.w);

        // To the shaded point
        float3 Lvec = light_pos - world_pos;
        float  d    = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;

        // Distance attenuation (your helper)
        float  attDist = attenuation(range, d);

        // Angular attenuation
        float  cosTheta = dot(normalize(-L), entity_forward); 
        float cosInnerEff = compute_cosInner(cosInner, cos_outer, g_default_spot_blend);
        float cone = spot_blend_hermite(cosTheta, cos_outer, cosInnerEff, g_softness_exp);

        // Final spotlight attenuation
        float  att = attDist * cone;
        float shadow_vis = 1.0;

        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            Texture2D shadow_map = sfg_get_texture2D(shadow_map_index);
            gpu_shadow_data sd = shadow_data_buffer[shadow_data_index];
            float4x4 light_space = sd.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;

            shadow_vis = sample_shadow_cone(shadow_map, smp_shadow, light_space, world_pos, N, L, entity_forward, shadow_resolution, cos_outer);
        }

        if (att > 0.0f)
        {
            float3 radiance = light_col * (intensity * att);
            lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);
        }
    }

    float linear_depth = linearize_depth(device_depth, rp_data.near_plane, rp_data.far_plane);
    int layer = -1;
    [loop]
    for(int i = 0; i < rp_data.cascades_count; i++)
    {
        if(linear_depth < float_buffer[rp_data.cascade_levels_gpu_index + i] )
        {
            layer = i;
            break;
        }
    }
    if(layer == -1)
    {
        layer = int(rp_data.cascades_count);
    }

    [loop]
    for (uint i = 0; i < dir_light_count; i++)
    {
        gpu_dir_light light = dir_light_buffer[i];
        gpu_entity e = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;

        float3 L = normalize(-e.forward.xyz);       

        float intensity = light.intensity.x; 

        float3 radiance = light_col * intensity; 

        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        float shadow_vis = 1.0f;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            Texture2DArray shadow_map = sfg_get_texture2DArray(shadow_map_index);
            gpu_shadow_data sd_curr = shadow_data_buffer[shadow_data_index + layer];
            float4x4 light_space_curr = sd_curr.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;
            shadow_vis = sample_cascade_shadow(
                    shadow_map, smp_shadow,
                    light_space_curr,
                    world_pos, N, L,
                    layer,
                    shadow_resolution,
                    sd_curr.texel_world
                );
        }

       // lighting = lighting * 0.0f + shadow_vis.xxx;
         lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);

    }

    lighting += emissive;

    //if(layer == 1)
    //return float4(0.2, 0,0,1);

    //return float4((float)layer / (float)rp_data.cascades_count,0,0,1);
    return float4(lighting, 1.0f);
}


