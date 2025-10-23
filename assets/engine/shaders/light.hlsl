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
