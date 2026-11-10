/* Retail formatter entry shared by ordinary and guest-mask varargs callers. */
#ifndef P4PC_EE_STDIO_H
#define P4PC_EE_STDIO_H
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
int p4pc_ee_vsnprintf(char *, size_t, const char *, unsigned, va_list);
int p4pc_ee_vfprintf(FILE *, const char *, unsigned, va_list);
#endif
