// HLSL: unpack two half-precision floats from one uint
float2 unpack_half2x16(uint packed)
{
    uint lo = packed & 0xFFFFu;
    uint hi = packed >> 16;
    return float2(f16tof32(lo), f16tof32(hi));
}