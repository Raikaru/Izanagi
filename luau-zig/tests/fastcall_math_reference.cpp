// Numerical samples against the pinned Luau VM's actual fastcall table.
// Build against the pinned VM; this is test-only and not linked into the Zig VM.
#include "lbuiltins.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

struct Case { unsigned id; unsigned argc; };
static const Case cases[] = {
    {2,1},{3,1},{4,1},{5,2},{6,1},{7,1},{8,1},{9,1},{10,1},{11,1},{12,1},{13,2},
    {14,1},{15,2},{16,1},{17,1},{18,2},{19,2},{20,1},{21,2},{22,1},{23,1},{24,1},
    {25,1},{26,1},{27,1},{28,2},{29,2},{30,1},{31,2},{32,2},{33,2},{34,3},
    {35,2},{36,2},{37,3},{38,2},{39,2},{46,3},{47,1},{48,1},{55,1},{56,1},
    {59,2},{64,1},{89,3},{91,1},{92,1},{93,1},
};
static const uint64_t edges[] = {
    0,0x8000000000000000ull,1,0x8000000000000001ull,0x7ff0000000000000ull,
    0xfff0000000000000ull,0x7ff8000000001234ull,0xfff8000000005678ull,
    0x41e0000000000000ull,0xc1e0000000000000ull,0x41f0000000000000ull,0x43e0000000000000ull
};
int main() {
    uint32_t dense_seed = 0x2345abcdu;
    for (const auto item : cases) {
        uint64_t seed = 0x8d26c5ab91d37e4full;
        for (unsigned i = 0; i < 10009; ++i) {
            double x;
            if (i == 0) x = 4;
            else if (i <= 8) {
                dense_seed = dense_seed * 1664525u + 1013904223u;
                const bool bitwise = (item.id >= 28 && item.id <= 39) || item.id == 55 || item.id == 56 || item.id == 59 || item.id == 64;
                int value = bitwise ? int(dense_seed % 2049) - 1024 : int(dense_seed % 19001) - 9500;
                x = bitwise ? double(value) : double(value) / 10000;
            } else {
                seed = seed * 6364136223846793005ull + 1442695040888963407ull;
                uint64_t bits = i - 9 < sizeof(edges) / sizeof(edges[0]) ? edges[i - 9] : seed;
                std::memcpy(&x, &bits, 8);
            }
            TValue input[3] = {}, output[2] = {};
            setnvalue(&input[0], x);
            setnvalue(&input[1], item.id == 59 ? 33 : 2);
            setnvalue(&input[2], 3);
            int count = (item.id == 14 || item.id == 20) ? 2 : 1;
            int got = luauF_table[item.id](nullptr, output, input, count, input + 1, item.argc);
            if (got != count) return 2;
            for (int j = 0; j < count; ++j) {
                uint64_t bits;
                if (ttisboolean(&output[j])) bits = bvalue(&output[j]) != 0;
                else { double v = nvalue(&output[j]); std::memcpy(&bits, &v, 8); }
                std::printf("M %u %u %d %016llx\n", item.id, i, j, (unsigned long long)bits);
            }
        }
    }
}
