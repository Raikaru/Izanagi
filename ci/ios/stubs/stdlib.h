#ifndef P4PC_STUB_STDLIB_H
#define P4PC_STUB_STDLIB_H
#include <stddef.h>
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
int atoi(const char *);
long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
double atof(const char *);
char *getenv(const char *);
void exit(int) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
int abs(int);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
#endif
