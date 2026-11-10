/* Retail 00450dd0 formats its variadic arguments with vsprintf into a stack buffer and draws the text
 * with the scale taken from fGpffffac58. MWCC reads the variadic save area through its
 * __builtin_args_info and __builtin_next_arg; clang's va_list is the standard way to reach the same
 * arguments. */
void sdkDbprtPrint(PackedVec2f position, f32 z, const void *format, ...)
{
    char buffer[0x100];
    union { f32 f; s32 bits; PackedColor4 color; } scale;
    va_list args;
    extern f32 fGpffffac58;

    scale.f = fGpffffac58;
    va_start(args, format);
    vsprintf(buffer, (const char *)format, args);
    va_end(args);
    sdkDbprtDrawText(scale.color, position, z, buffer);
}
