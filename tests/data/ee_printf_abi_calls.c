/* Compile this source through the real adapter, not a hand-packed oracle call.
 * Sint32/Uint32 intentionally stay narrow at every variadic call expression. */
typedef signed int Sint32;
typedef unsigned int Uint32;
typedef unsigned long long Uint64;
extern int func_00442088(char *, const char *, ...);
extern int retail_sprintf(char *, const char *, ...) __asm__("func_00442088");

int p4_printf_abi_words(char *out, const char *fmt, Sint32 s, Uint32 u)
{
    return func_00442088(out, fmt, s, u, s, u, s, u, (short)s, (unsigned short)u);
}

int p4_printf_abi_mixed(char *out, const char *fmt, Sint32 s, Uint32 guest,
                        char *native, Uint64 soft, double real, Sint32 width,
                        Sint32 precision, Sint32 character)
{
    return func_00442088(out, fmt, width, precision, s, native, guest, soft,
                        real, native, guest, character, s);
}

int p4_printf_abi_alias(char *out, const char *fmt, Sint32 s, Uint32 u,
                        char *native, double real)
{
    return retail_sprintf(out, fmt, s, u, native, real);
}
