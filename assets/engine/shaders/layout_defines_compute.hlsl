#define global_cbv register(b0, space0)
#define constants_cbv register(b1, space0)

#define static_sampler_anisotropic register(s0, space0)
#define static_sampler_linear register(s1, space0)
#define static_sampler_nearest register(s2, space0)

cbuffer sfg_globals : register(b0, space0)
{
    float f1;
    float f2;
}

cbuffer sfg_constants : register(b1, space0)
{
    uint sfg_rp_ubo_index;
    uint sfg_material_ubo_index;
    uint sfg_texture_ubo_index;
    uint sfg_rp_constant0;
    uint sfg_rp_constant1;
    uint sfg_rp_constant2;
    uint sfg_rp_constant3;
    uint sfg_rp_constant4;
    uint sfg_rp_constant5;
    uint sfg_rp_constant6;
    uint sfg_rp_constant7;
    uint sfg_rp_constant8;
    uint sfg_rp_constant9;
    uint sfg_rp_constant10;
    uint sfg_object_constant0;
    uint sfg_object_constant1;
    uint sfg_object_constant2;
    uint sfg_object_constant3;
    uint sfg_object_constant4;
    uint sfg_object_constant5;
    uint sfg_object_constant6;
    uint sfg_object_constant7;
    uint sfg_object_constant8;
}

template<typename T>
ConstantBuffer<T> sfg_get_cbv(uint index)
{
    ConstantBuffer<T> b = ResourceDescriptorHeap[index];
    return b;
}

template<typename T>
StructuredBuffer<T> sfg_get_ssbo(uint index)
{
    StructuredBuffer<T> b = ResourceDescriptorHeap[index];
    return b;
}

Texture2D sfg_get_texture2D(uint index)
{
    Texture2D txt = ResourceDescriptorHeap[index];
    return txt;
}

Texture2D<float> sfg_get_texture2Df(uint index)
{
    Texture2D<float> txt = ResourceDescriptorHeap[index];
    return txt;
}
RWTexture2D<float> sfg_get_rwtexture2D(uint index)
{
    RWTexture2D<float> txt = ResourceDescriptorHeap[index];
    return txt;
}


TextureCube sfg_get_texturecube(uint index)
{
    TextureCube txt = ResourceDescriptorHeap[index];
    return txt;
}

Texture2DArray sfg_get_texture2DArray(uint index)
{
    Texture2DArray txt = ResourceDescriptorHeap[index];
    return txt;
}


SamplerState sfg_get_sampler_state(uint index)
{
    SamplerState ss = SamplerDescriptorHeap[index];
    return ss;
}

