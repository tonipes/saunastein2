#define global_cbv register(b0, space0)
#define constants_cbv register(b1, space0)

#define static_sampler_anisotropic register(s0, space0)
#define static_sampler_anisotropic_repeat register(s1, space0)
#define static_sampler_linear register(s2, space0)
#define static_sampler_linear_repeat register(s3, space0)
#define static_sampler_nearest register(s4, space0)
#define static_sampler_nearest_repeat register(s5, space0)

#define static_sampler_gui_default register(s6, space0)
#define static_sampler_gui_text register(s7, space0)
#define static_sampler_shadow_2d register(s8, space0)
#define static_sampler_shadow_cube register(s9, space0)

cbuffer sfg_globals : register(b0, space0)
{
    float f1;
    float f2;
}

cbuffer sfg_constants : register(b1, space0)
{
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
    uint sfg_rp_constant11;
    uint sfg_rp_constant12;
    uint sfg_mat_constant0;
    uint sfg_mat_constant1;
    uint sfg_mat_constant2;
    uint sfg_mat_constant3;
    uint sfg_mat_constant4;
    uint sfg_mat_constant5;
    uint sfg_object_constant0;
    uint sfg_object_constant1;
    uint sfg_object_constant2;
    uint sfg_object_constant3;
    uint sfg_object_constant4;
    uint sfg_object_constant5;
    uint sfg_object_constant6;
    uint sfg_object_constant7;
    uint sfg_object_constant8;
    uint sfg_object_constant9;
    uint sfg_object_constant10;
    uint sfg_object_constant11;
    uint sfg_object_constant12;
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

template<typename T>
T sfg_get_texture(uint index)
{
    T txt = ResourceDescriptorHeap[index];
    return txt;
}

SamplerState sfg_get_sampler_state(uint index)
{
    SamplerState ss = SamplerDescriptorHeap[index];
    return ss;
}

