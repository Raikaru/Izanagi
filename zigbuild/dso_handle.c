/* Executable crtbegin semantics: the atexit wrapper loads this zero value. */
__attribute__((visibility("hidden"), section(".data")))
void *__dso_handle = 0;
