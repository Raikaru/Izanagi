#include "p4pc/content_digest.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "content_digest:%d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static int vector(const char *input, const char *expected)
{
    size_t length = strlen(input);
    struct { unsigned char before; char hex[65]; unsigned char after; } out;
    unsigned char unaligned[130];
    P4ContentDigest state;
    CHECK(length < sizeof unaligned - 1);
    memcpy(unaligned + 1, input, length);
    memset(&out, 0xa5, sizeof out);
    p4_content_digest(unaligned + 1, length, out.hex);
    CHECK(!strcmp(out.hex, expected));
    CHECK(out.hex[64] == 0 && out.before == 0xa5 && out.after == 0xa5);
    for (size_t chunk = 1; chunk <= 97; ++chunk) {
        p4_content_digest_begin(&state);
        p4_content_digest_update(&state, NULL, 0);
        for (size_t offset = 0; offset < length;) {
            size_t count = length - offset;
            if (count > chunk) count = chunk;
            p4_content_digest_update(&state, unaligned + 1 + offset, count);
            offset += count;
        }
        p4_content_digest_update(&state, NULL, 0);
        p4_content_digest_end(&state, out.hex);
        CHECK(!strcmp(out.hex, expected));
        CHECK(out.before == 0xa5 && out.after == 0xa5);
    }
    return 0;
}

int main(void)
{
    char hex[65];
    unsigned char block[1000];
    P4ContentDigest state;
    P4ContentFingerprint fingerprint, untouched;
    unsigned char source[] = {0, 1, 2, 3, 4, 5};
    unsigned char loaded[sizeof source];
    memset(&fingerprint, 0xa5, sizeof fingerprint);
    untouched = fingerprint;
    CHECK(p4_content_fingerprint_seal(&fingerprint, NULL, sizeof source) == -1);
    CHECK(p4_content_fingerprint_seal(&fingerprint, source, 0) == -1);
    CHECK(!memcmp(&fingerprint, &untouched, sizeof fingerprint));
    CHECK(!p4_content_fingerprint_seal(&fingerprint, source, sizeof source));
    memcpy(loaded, source, sizeof source);
    CHECK(p4_content_fingerprint_matches(&fingerprint, loaded, sizeof loaded));
    CHECK(!p4_content_fingerprint_matches(&fingerprint, loaded, sizeof loaded - 1));
    CHECK(!p4_content_fingerprint_matches(&fingerprint, NULL, sizeof loaded));
    CHECK(!p4_content_fingerprint_matches(NULL, loaded, sizeof loaded));
    loaded[3] ^= 1;
    CHECK(!p4_content_fingerprint_matches(&fingerprint, loaded, sizeof loaded));
    loaded[3] ^= 1;
    CHECK(p4_content_fingerprint_matches(&fingerprint, loaded, sizeof loaded));
    CHECK(!p4_content_fingerprint_matches(&fingerprint,
        (const void *)(uintptr_t)(UINTPTR_MAX - 1), sizeof loaded));
    CHECK(!vector("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    CHECK(!vector("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    CHECK(!vector("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    CHECK(!vector("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
        "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"));
    p4_content_digest(NULL, 0, hex);
    CHECK(!strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    memset(block, 'a', sizeof block);
    p4_content_digest_begin(&state);
    for (unsigned i = 0; i < 1000; ++i)
        p4_content_digest_update(&state, block, sizeof block);
    p4_content_digest_end(&state, hex);
    CHECK(!strcmp(hex, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    return 0;
}
