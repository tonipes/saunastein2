float linearize_depth(float depth, float nearZ, float farZ) 
{
    float z = (1.0 - depth) * 2.0 - 1.0; // reverse z flip
    return (2.0 * nearZ * farZ) / (farZ + nearZ - z * (farZ - nearZ));
}

static float3 reconstruct_world_position(float2 uv, float device_depth, float4x4 inv_view_proj)
{
    // NDC y is Y up, uv Y is Y down.
    float2 ndcXY = float2(uv.x * 2.0f - 1.0f,
                      1.0f - uv.y * 2.0f);
                      
    // depth is 0-1 already DX NDC.
    float  ndcZ  = device_depth;
    
    float4 ndc = float4(ndcXY, ndcZ, 1.0);

    // in world space
    ndc = mul(inv_view_proj, ndc);
    ndc /= ndc.w;

    return ndc.xyz;
}