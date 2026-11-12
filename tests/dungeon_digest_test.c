/* The cross-target dungeon digest (p4walk_dungeon_digest) hashes guest data, not where it sits or RwMatrix padding: moving a unit's matrix block or
 * changing its pad words leaves the row equal, a changed row/position/generator word changes exactly its component. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/walk_script.h"

#define LOW 0x00700000u
#define HIGH 0x01100000u
#define SPAN 0x00200000u
static uint32_t low[SPAN / 4], high[SPAN / 4];
static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "dungeon_digest: line %d: %s\n", __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static uint32_t *word(uint32_t address)
{
    if (address >= LOW && address < LOW + SPAN) return &low[(address - LOW) / 4];
    if (address >= HIGH && address < HIGH + SPAN) return &high[(address - HIGH) / 4];
    return NULL;
}

static void put(uint32_t address, uint32_t value)
{
    *word(address) = value;
}

static uint32_t read32(void *user, uint32_t address)
{
    const uint32_t *w = word(address);

    (void)user;
    return w != NULL ? *w : 0;
}

#define TASK 0x00700100u
#define PLAYER_WORK 0x00700200u
#define PLAYER_MATRIX 0x01100000u
#define UNIT 0x01100100u
#define UNIT_MATRIX 0x01100400u
#define MOVED_MATRIX 0x01100800u
#define RNG_BASE 0x00700400u

static void matrix(uint32_t m, float x)
{
    for (uint32_t k = 0; k < 16; k++) {
        float f = x + (float)k;
        uint32_t u;

        memcpy(&u, &f, 4);
        put(m + 4u * k, u);
    }
}

/* field 42:0 step 41 seed 0x10c5 on a two-cell floor, the rotate pc task with its matrix, one shadow unit, the additive generator and the MT array */
static void setup(void)
{
    static const char name[24] = "rotate pc";

    memset(low, 0, sizeof low);
    memset(high, 0, sizeof high);
    put(0x00764ADCu, TASK);
    for (uint32_t k = 0; k < 6; k++) {
        uint32_t v;

        memcpy(&v, name + 4 * k, 4);
        put(TASK + 4u * k, v);
    }
    put(TASK + 0x38, PLAYER_WORK);
    put(PLAYER_WORK + 0x48, PLAYER_MATRIX);
    matrix(PLAYER_MATRIX, 1.0f);
    put(0x007D2510u, 0x00700300u);
    put(0x00700300u + 0x38, 0x00700340u);
    put(0x00700340u + 0x18, 42u);
    put(0x00764334u, 0x00700380u);
    put(0x00700380u + 0x38, 0x007003C0u);
    put(0x007003C0u + 4, 41u);
    put(0x00764330u, 0x10C5u);
    put(0x007D2564u, 0x31230001u);
    put(0x007D2568u, 0x00020007u);
    put(0x007D256Cu, 0x000A0000u);
    put(0x007D2574u, 0x31240001u);
    put(0x00764708u, 0x10u);
    put(0x008872F0u, RNG_BASE);
    put(0x008872F4u, RNG_BASE + 12u);
    put(0x008872F8u, RNG_BASE + 4u);
    for (uint32_t k = 0; k < 31; k++) put(RNG_BASE + 4u * k, 0x1000u + k);
    for (uint32_t k = 0; k < 0x270u; k++) put(0x00885A90u + 4u * k, k * 2654435761u);
    put(0x00762EA0u, 0x00700500u);
    put(0x00700500u + 8, 0x00700600u);
    put(0x00700600u + 4, UNIT);
    put(UNIT, 0x464u);
    put(UNIT + 0x164, UNIT_MATRIX);
    matrix(UNIT_MATRIX, 100.0f);
}

static void row(uint32_t frame, char *line, size_t n)
{
    const P4WalkHost host = { .read32 = read32 };
    uint32_t roll = 2166136261u;

    CHECK(p4walk_dungeon_digest(&host, frame, 100, &roll, line, n) == 1);
}

static const char *field(const char *line, const char *key)
{
    const char *p = strstr(line, key);

    return p != NULL ? p + strlen(key) : "";
}

static int same(const char *a, const char *b, const char *key)
{
    const char *x = field(a, key), *y = field(b, key);

    return strncmp(x, y, strcspn(x, " ")) == 0;
}

int main(void)
{
    static const char prefix[] = "dungeon digest flip 200 field 42:0 step 41 seed 000010c5 grid ";
    char base[320], other[320];
    const P4WalkHost host = { .read32 = read32 };
    uint32_t roll = 2166136261u;

    setup();
    row(200, base, sizeof base);
    CHECK(strncmp(base, prefix, sizeof prefix - 1) == 0);
    CHECK(strstr(base, " units 1:") != NULL && strstr(base, " battle -1 ") != NULL);
    CHECK(p4walk_dungeon_digest(&host, 150, 100, &roll, other, sizeof other) == 0);

    /* RwMatrix pad words are stack garbage the game never initialises: not state */
    put(PLAYER_MATRIX + 0x1C, 0x00007FF7u);
    put(UNIT_MATRIX + 0x2C, 3u);
    put(UNIT_MATRIX + 0x3C, 0x202u);
    row(200, other, sizeof other);
    CHECK(strcmp(base, other) == 0);

    /* where a block sits is not state either (a 64-bit host's heap need not place it at the same guest address) */
    for (uint32_t k = 0; k < 16; k++) put(MOVED_MATRIX + 4u * k, read32(NULL, UNIT_MATRIX + 4u * k));
    put(UNIT + 0x164, MOVED_MATRIX);
    row(200, other, sizeof other);
    CHECK(strcmp(base, other) == 0);

    /* one ulp of a unit's position is */
    put(MOVED_MATRIX + 0x30, read32(NULL, MOVED_MATRIX + 0x30) + 1u);
    row(200, other, sizeof other);
    CHECK(!same(base, other, " units "));
    CHECK(same(base, other, " player ") && same(base, other, " rng ") && same(base, other, " grid "));
    put(MOVED_MATRIX + 0x30, read32(NULL, MOVED_MATRIX + 0x30) - 1u);

    /* one generator word changes the generator and the rolling hash, nothing else */
    put(RNG_BASE + 20u, 0xDEADu);
    row(200, other, sizeof other);
    CHECK(!same(base, other, " rng ") && !same(base, other, " roll "));
    CHECK(same(base, other, " units ") && same(base, other, " player ") && same(base, other, " mt "));
    put(RNG_BASE + 20u, 0x1005u);

    /* a placed cell's exits are the floor's layout */
    put(0x007D256Cu, 0x000B0000u);
    row(200, other, sizeof other);
    CHECK(!same(base, other, " grid ") && same(base, other, " rng "));

    if (failures == 0) printf("dungeon_digest: PASS\n");
    return failures != 0;
}
