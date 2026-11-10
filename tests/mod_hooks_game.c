#include <stdint.h>
typedef unsigned int u32;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;

// FUN_00100100
u32 mod_test_unsigned(u32 value)
{
    return value + 1;
}

// FUN_00100120
s16 mod_test_narrow(s8 value, u16 add)
{
    return (s16)(value + add);
}

// FUN_00100140
float mod_test_mixed(int value, float real, unsigned char *byte, double wide)
{
    return (float)(value + real + *byte + wide);
}

// FUN_00100160
u32 *mod_test_pointer(u32 *value)
{
    return value;
}

// FUN_00100180
int mod_test_recursive(int depth)
{
    return depth ? mod_test_recursive(depth - 1) + 1 : 0;
}

// FUN_001001A0
void mod_test_void(u32 *value)
{
    *value += 1;
}
