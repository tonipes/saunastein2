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
