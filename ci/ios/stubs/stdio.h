/* The libc declarations the iOS context files use (Darwin's signatures), for the SDK-less compile check. */
#ifndef P4PC_STUB_STDIO_H
#define P4PC_STUB_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct __sFILE FILE;
extern FILE *__stderrp;
#define stderr __stderrp
int snprintf(char *, size_t, const char *, ...) __attribute__((format(printf, 3, 4)));
int fprintf(FILE *, const char *, ...) __attribute__((format(printf, 2, 3)));
int puts(const char *);
#endif
