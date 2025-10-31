struct gpu_point_light
{
    float4 color_entity_index;
    float4 intensity_range;
    float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
};

struct gpu_spot_light
{
   float4 color_entity_index;
   float4 intensity_range_inner_outer;
    float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
};

struct gpu_dir_light
{
    float4 color_entity_index;
    float4 intensity;
    float4 shadow_resolution_map_and_data_index;    // xy res, z map index, w data index
};

struct gpu_shadow_data
{
    float4x4 light_space_matrix;
    float texel_world;
};

// d = distance to light
float get_range_attenuation(float r, float d)
{
    if (r <= 0.0) return 1.0;
    float x = saturate(1.0 - (d*d) / (r*r)); // 1 - (d/r)^2   in [0,1]
    return x * x;                             // smooth rolloff
}

float attenuation(float r, float d)
{
    return get_range_attenuation(r, d) / max(d*d, 1e-4); // inverse-square * clamp
}

float spot_smooth(float cosTheta, float cosInner, float cosOuter)
{
    // Avoid div by 0 if inner==outer
    float denom = max(cosInner - cosOuter, 1e-4);
    float x = saturate((cosTheta - cosOuter) / denom);
    return x * x;
}

static const float g_default_spot_blend = 0.2; 
static const float g_softness_exp       = 1.0;    // >1 = sharper, <1 = softer

float compute_cosInner(float cosInnerPacked, float cosOuter, float blend)
{
    if (cosInnerPacked > cosOuter + 1e-5) return cosInnerPacked;

    float thetaO = acos(saturate(cosOuter));
    float thetaI = thetaO * (1.0 - saturate(blend));
    return cos(thetaI);
}

float spot_blend_hermite(float cosTheta, float cosOuter, float cosInner, float softnessExp)
{
    float denom = max(cosInner - cosOuter, 1e-4);
    float x = saturate((cosTheta - cosOuter) / denom);
    // hermite
    x = x * x * (3.0 - 2.0 * x);
    // optional shaping
    return pow(x, max(softnessExp, 1e-3));
}

