struct gpu_point_light
{
    float4 color_entity_index;
    float4 intensity_range;
};

struct gpu_spot_light
{
   float4 color_entity_index;
   float4 intensity_range_inner_outer;
};

struct gpu_dir_light
{
    float4 color_entity_index;
    float4 intensity_range;
};

// d = distance to light, r = glTF "range" (meters). 
float getRangeAttenuation(float r, float d)
{
    if (r <= 0.0) return 1.0;
    float x = saturate(1.0 - (d*d) / (r*r)); // 1 - (d/r)^2   in [0,1]
    return x * x;                             // smooth rolloff
}

float attenuation(float r, float d)
{
    return getRangeAttenuation(r, d) / max(d*d, 1e-4); // inverse-square * clamp
}