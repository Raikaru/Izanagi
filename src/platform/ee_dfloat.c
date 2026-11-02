/* Native-double call ABI for the retail raw-bit double-libm cores.
 * Reinterpretation only: the algorithm and every arithmetic step run in
 * ee_dlibm.c / ps2_fpbit.c, independent of host libm and FPU rounding state.
 */
#include <stdint.h>
#include <string.h>
#include "p4pc/ee_dlibm.h"

static uint64_t to_bits(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static double from_bits(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

double p4pc_ee_dsqrt(double value)
{
    return from_bits(func_00448a00(to_bits(value)));
}

double p4pc_ee_pow(double x, double y)
{
    return from_bits(func_00447d00(to_bits(x), to_bits(y)));
}

double p4pc_ee_log(double value)
{
    return from_bits(func_004476b0(to_bits(value)));
}

double p4pc_ee_log10(double value)
{
    return from_bits(func_00447b88(to_bits(value)));
}

double p4pc_ee_acos(double value)
{
    return from_bits(func_004470f0(to_bits(value)));
}
