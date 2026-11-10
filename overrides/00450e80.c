/* Retail 00450e80: as sdkDbprtPrint, with the colour passed in. The variadic arguments are read with
 * va_list instead of MWCC's __builtin_args_info / __builtin_next_arg. */
void sdkDbprtPrintColor(PackedColor4 color, PackedVec2f position, f32 z,
                   const void *format, ...)
{
    char buffer[0x100];
    va_list args;

    va_start(args, format);
    vsprintf(buffer, (const char *)format, args);
    va_end(args);
    sdkDbprtDrawText(color, position, z, buffer);
}
