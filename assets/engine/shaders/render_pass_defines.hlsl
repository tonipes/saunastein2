struct render_pass_data_opaque
{
    float4x4 view_proj;
};

struct render_pass_data_forward
{
    float4x4 view_proj;
    float4  ambient;
    float4 camera_pos;
    float2  resolution;         
    float2  proj_params;       // (tanHalfFovY, orthoHalfHeight)
};