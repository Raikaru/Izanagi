/* 002b32d0 fclExpandNameTags: expand <P1> <P2> <R1> <R2> in a text buffer from four replacement strings
 * (the Fusion Forecast condition lines, "Use a Persona of the <R1> Arcana"). Retail copies the four strings to four
 * 0x80-byte locals declared one after the other and indexes the second to fourth as `first + tag * 0x80`, which relies on
 * MWCC laying the four arrays out back to back. A host compiler lays separate locals out as it likes, so every tag but
 * <P1> read other memory and expanded to nothing ("Use a Persona of the  Arcana"). The four copies live in one array here,
 * so the same index reads the intended string. The expansion itself is retail's: for each tag in turn (the tag strings are
 * the words at 0x0063F110, their lengths the bytes at 0x007488C0), append the converted text before it and the
 * replacement, drop the tag, then append the converted tail. Copies are bounded by their buffers (retail's are not; no
 * retail text or name reaches the bounds). Golden mode expands through p4_golden_expand_name_tags, which takes the four
 * names directly and bounds every write by the caller's 128-byte buffer. That branch is also what config/mirror_fixes.txt
 * adds to the head of the mirror's retail body; this override replaces the whole definition, so the copy here is the one
 * that runs and the mirror_fixes row is superseded (keep the two in step). */
#include "p4pc/game_mode.h"
#include "p4pc/golden_tag_expand.h"
#include "p4pc/package_boot.h"
extern void frFontExCopyString(void *, void *, s32);
extern s32 func_00443010(u8 *, u8 *);
extern s32 fclTagTextLength(u8 *, s64);

static void p4_copy_name_002b32d0(u8 *dest, u8 *name)
{
    s32 i;

    for (i = 0; i < 0x7F && name[i] != 0; i++) {
        dest[i] = name[i];
    }
    dest[i] = 0;
}

u8 *fclExpandNameTags(u8 *text, u8 *firstName, u8 *secondName, u8 *thirdName, u8 *fourthName)
{
    u8 replacements[4 * 0x80];
    u8 expanded[0x40];
    u8 prefix[0x40];
    u8 converted[0x40];
    u8 *tag;
    u8 *dest;
    u8 *rest;
    s32 outer;
    s32 size;
    s32 offset;
    s32 i;
    s32 used;
    s8 offsets[4];

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        for (i = 0; i < 4; i++) {
            offsets[i] = *(s8 *)(0x007488C0 + i);
        }
        if (!p4_golden_expand_name_tags(text, firstName, secondName, thirdName, fourthName, offsets))
            p4_package_boot_runtime_failed("Golden fusion name expansion exceeds the 128-byte text buffer");
        return text;
    }
    p4_copy_name_002b32d0(replacements, firstName);
    p4_copy_name_002b32d0(replacements + 0x80, secondName);
    p4_copy_name_002b32d0(replacements + 0x100, thirdName);
    p4_copy_name_002b32d0(replacements + 0x180, fourthName);
    for (i = 0; i < 0x40; i++) {
        expanded[i] = 0;
    }
    used = 0;
    outer = 0;
    while (outer < 4) {
        tag = *(u8 **)(0x0063F110 + outer * 4);
        if (func_00443010(text, tag) == 0) {
            outer++;
            continue;
        }
        dest = replacements + outer * 0x80;
        offset = *(s8 *)(0x007488C0 + outer);
        do {
            for (i = 0; i < 0x40; i++) {
                prefix[i] = 0;
                converted[i] = 0;
            }
            size = fclTagTextLength(text, (s8)outer);
            for (i = 0; i < size && i < 0x3F; i++) {
                prefix[i] = text[i];
            }
            frFontExCopyString(prefix, converted, 0x40);
            for (i = 0; converted[i] != 0 && used < 0x3F; i++) {
                expanded[used++] = converted[i];
            }
            for (i = 0; dest[i] != 0 && used < 0x3F; i++) {
                expanded[used++] = dest[i];
            }
            expanded[used] = 0;
            /* retail strcpy(text, text + size + offset): a forward copy over itself */
            rest = text + size + offset;
            for (i = 0; (text[i] = rest[i]) != 0; i++) {
            }
        } while (func_00443010(text, tag) != 0);
    }
    frFontExCopyString(text, converted, 0x40);
    for (i = 0; converted[i] != 0 && used < 0x3F; i++) {
        expanded[used++] = converted[i];
    }
    expanded[used] = 0;
    for (i = 0; (text[i] = expanded[i]) != 0; i++) {
    }
    return text;
}
