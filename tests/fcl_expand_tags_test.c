/* overrides/002b32d0.c (fclExpandNameTags) against scripted tag tables and font helpers. The Fusion Forecast
 * conditions ("Use a Persona of the <R1> Arcana", "Use <P1> and <P2>", "Use <R1> and <R2> Personas") expand their
 * tags from four replacement strings; retail finds the second to fourth at `first + tag * 0x80`, which a host
 * compiler's own local layout broke (every tag but <P1> expanded to nothing). Each tag must read its own string.
 * Expected texts are worked from the retail routine's description (override header), not from the override. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "p4pc/os.h"
#include "p4pc/game_mode.h"
#include "guest_test.h"

typedef uint8_t u8;
typedef int8_t s8;
typedef uint32_t u32;
typedef int32_t s32;

/* Tag strings 0x0063F110 (four guest words), tag lengths 0x007488C0 (four bytes). */
#define TAG_WORDS 0x0063F110u
#define TAG_LENGTHS 0x007488C0u
#define G32(addr) (*(volatile u32 *)P4_G2H((uint32_t)(addr)))
#define GADDR(x) ((u32)P4_H2G(x))

u8 *fclExpandNameTags(u8 *text, u8 *firstName, u8 *secondName, u8 *thirdName, u8 *fourthName);

static int checks, failures, copy_calls;

#define CHECK(cond, ...)                                          \
    do {                                                          \
        checks++;                                                 \
        if (!(cond)) {                                            \
            failures++;                                           \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);  \
            fprintf(stderr, __VA_ARGS__);                         \
            fputc('\n', stderr);                                  \
        }                                                         \
    } while (0)

/* The override branches on the game mode (Golden keeps its bounded expansion) and refuses the session on overflow. */
static P4GameMode scripted_mode = P4_GAME_MODE_PS2;
static int boot_failures;

P4GameMode p4_game_mode(void)
{
    return scripted_mode;
}

void p4_package_boot_runtime_failed(const char *reason)
{
    (void)reason;
    boot_failures++;
}

static const char *const tag_text[4] = {"<P1>", "<P2>", "<R1>", "<R2>"};

/* retail 00443010 is strstr */
s8 *func_00443010(s8 *haystack, s8 *needle)
{
    return (s8 *)strstr((const char *)haystack, (const char *)needle);
}

/* The text length before the next tag of the mode. Retail also steps over two-byte characters; the
 * scripted texts are ASCII. */
s32 fclTagTextLength(u8 *text, long long mode)
{
    const char *at = strstr((const char *)text, tag_text[(int)mode & 3]);
    return at != NULL ? (s32)(at - (const char *)text) : (s32)strlen((const char *)text);
}

/* The font conversion, scripted as an upper-casing so a converted piece is visible in the result:
 * retail converts the text around a tag and appends the replacement as it is. */
void frFontExCopyString(void *source, void *destination, s32 limit)
{
    const u8 *in = source;
    u8 *out = destination;
    s32 i;

    copy_calls++;
    for (i = 0; i < limit - 1 && in[i] != 0; i++) {
        out[i] = (u8)(in[i] >= 'a' && in[i] <= 'z' ? in[i] - 'a' + 'A' : in[i]);
    }
    out[i] = 0;
}

static u8 *tags_at[4];

static void set_tables(void)
{
    static const u8 lengths[4] = {4, 4, 4, 4};
    int i;

    for (i = 0; i < 4; i++) {
        G32(TAG_WORDS + 4 * i) = GADDR(tags_at[i]);
        *(u8 *)P4_G2H(TAG_LENGTHS + i) = lengths[i];
    }
}

static void expand_is(const char *label, const char *input, const char *expected)
{
    u8 text[0x80];
    u8 first[0x15] = "Alpha", second[0x15] = "Beta", third[0x15] = "Magician", fourth[0x15] = "Priestess";
    u8 *result;

    memset(text, 0, sizeof text);
    strcpy((char *)text, input);
    result = fclExpandNameTags(text, first, second, third, fourth);
    CHECK(result == text, "%s: returns its text buffer", label);
    CHECK(strcmp((const char *)text, expected) == 0, "%s: '%s' -> '%s', expected '%s'", label, input, (const char *)text,
          expected);
    CHECK(strcmp((const char *)first, "Alpha") == 0 && strcmp((const char *)fourth, "Priestess") == 0,
          "%s: the names are not written", label);
}

static int run_test(int argc, char **argv)
{
    char tag_storage[4][8]; /* guest stack: the table holds their guest addresses */
    int i;

    (void)argc;
    (void)argv;
    if (p4t_guest_map(TAG_WORDS, 0x1000) != 0 || p4t_guest_map(TAG_LENGTHS, 0x1000) != 0) {
        fprintf(stderr, "fcl_expand_tags_test: cannot map the tag tables\n");
        return 1;
    }
    for (i = 0; i < 4; i++) {
        strcpy(tag_storage[i], tag_text[i]);
        tags_at[i] = (u8 *)tag_storage[i];
    }
    set_tables();

    /* The Fusion Forecast conditions. Text before and after a tag is converted, the name is not. */
    expand_is("no tag", "Use a Persona of the", "USE A PERSONA OF THE");
    expand_is("P1", "Use <P1>", "USE Alpha");
    expand_is("P1 alone", "<P1>", "Alpha");
    expand_is("P2", "and <P2>", "AND Beta");
    expand_is("R1 (the missing arcana name)", "<R1> Arcana", "Magician ARCANA");
    expand_is("R1 inside a line", "the <R1> Arcana", "THE Magician ARCANA");
    expand_is("R1 first of two", "Use <R1> and", "USE Magician AND");
    expand_is("R2", "<R2> Personas", "Priestess PERSONAS");
    expand_is("all four, in tag order", "<P1> <P2> <R1> <R2>", "Alpha Beta Magician Priestess");
    expand_is("empty text", "", "");
    CHECK(copy_calls > 0, "the font conversion ran");

    /* An empty replacement stays empty (the retail copy of a zero-length name), the others keep theirs. */
    {
        u8 text[0x80] = "<P1>|<P2>|<R1>|<R2>";
        u8 none[2] = "";
        u8 b[0x15] = "B", c[0x15] = "C", d[0x15] = "D";

        fclExpandNameTags(text, none, b, c, d);
        CHECK(strcmp((const char *)text, "|B|C|D") == 0, "empty first name: '%s'", (const char *)text);
    }

    /* Golden mode: the same expansion through the bounded helper, one name per tag; an overflowing expansion refuses the
     * session and leaves the text. */
    scripted_mode = P4_GAME_MODE_GOLDEN;
    expand_is("golden all four", "<P1> <P2> <R1> <R2>", "Alpha Beta Magician Priestess");
    expand_is("golden R1", "the <R1> Arcana", "the Magician Arcana");
    CHECK(boot_failures == 0, "golden expansions within the buffer do not fail the session");
    {
        u8 text[0x80];
        u8 big[0x15] = "ABCDEFGHIJKLMNOPQRST";
        int i;

        memset(text, 0, sizeof text);
        for (i = 0; i < 8; i++) {
            strcat((char *)text, "<R1>");
        }
        fclExpandNameTags(text, big, big, big, big);
        CHECK(boot_failures == 1, "an expansion past 128 bytes refuses the session (%d)", boot_failures);
    }
    scripted_mode = P4_GAME_MODE_PS2;
    printf("fcl_expand_tags: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
