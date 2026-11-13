#include "../tools/p4mod_zip32.h"
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "ZIP32 boundary failed at line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void)
{
    P4ModZip32 zip = {0, 0, 0};
    CHECK(p4mod_zip32_size_ok((uint64_t)UINT32_MAX - 1));
    CHECK(!p4mod_zip32_size_ok(UINT32_MAX));
    CHECK(p4mod_zip32_add(&zip, UINT16_MAX, 0, 0));
    P4ModZip32 before = zip;
    CHECK(!p4mod_zip32_add(&zip, (uint64_t)UINT16_MAX + 1, 0, 0));
    CHECK(zip.local_bytes == before.local_bytes && zip.directory_bytes == before.directory_bytes && zip.entries == before.entries);
    CHECK(!p4mod_zip32_add(&zip, 1, UINT32_MAX, 0));
    CHECK(!p4mod_zip32_add(&zip, 1, 0, UINT32_MAX));

    P4ModZip32 uncompressed = {0, 0, 0};
    CHECK(p4mod_zip32_add(&uncompressed, 1, (uint64_t)UINT32_MAX - 1, 0));
    P4ModZip32 count = {0, 0, UINT16_MAX - 2u};
    CHECK(p4mod_zip32_add(&count, 0, 0, 0));
    CHECK(count.entries == UINT16_MAX - 1u);
    CHECK(!p4mod_zip32_add(&count, 0, 0, 0));

    /* Local and central lengths each fit, but their sum plus the 22-byte
     * end record must fit as well. Exercise the last valid and first invalid
     * totals without allocating the corresponding archive. */
    P4ModZip32 near_local = {(uint64_t)UINT32_MAX - 99u, 0, 0};
    CHECK(p4mod_zip32_add(&near_local, 0, 0, 0));
    CHECK(near_local.local_bytes + near_local.directory_bytes + 22u == (uint64_t)UINT32_MAX - 1u);
    P4ModZip32 full_local = {(uint64_t)UINT32_MAX - 98u, 0, 0};
    CHECK(!p4mod_zip32_add(&full_local, 0, 0, 0));
    P4ModZip32 full_directory = {0, (uint64_t)UINT32_MAX - 98u, 0};
    CHECK(!p4mod_zip32_add(&full_directory, 0, 0, 0));
    return 0;
}
