/* Test-only symbol rename; production ISO parser and bytes are unchanged. */
#define iso_walk p4_test_actual_iso_walk
#include "../src/platform/files/iso9660.c"
