/* Exercise the real registry and its private allocation cursor directly, so
 * boundary cases do not require billions of registrations or a game run. */
#include <stdint.h>
#include <stdio.h>
#include "../src/platform/guest.c"

static void fn_a(void) {}
static void fn_b(void) {}
static void fn_c(void) {}
static void fn_d(void) {}

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "handle bounds line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

int main(void)
{
    uint32_t first = p4_fn_h2g((void *)fn_a);
    CHECK(first == HANDLE_BASE);
    CHECK(p4_fn_g2h(first) == (void *)fn_a);
    CHECK(p4_fn_h2g((void *)fn_a) == first);

    uint32_t collision = next_handle;
    p4_fn_register(collision, (void *)fn_b);
    CHECK(p4_fn_h2g((void *)fn_c) == 0);
    CHECK(next_handle == collision);
    CHECK(p4_fn_g2h(collision) == (void *)fn_b);

    char window[8];
    next_handle = HANDLE_BASE + 0x100u;
    p4_guest_window(next_handle + 1u, window, sizeof window);
    CHECK(p4_fn_h2g((void *)fn_c) == 0);
    CHECK(next_handle == HANDLE_BASE + 0x100u);
    CHECK(!p4_fn_known(next_handle));

    next_handle = HANDLE_BASE + 0x200u;
    p4_guest_window(next_handle - 1u, window, sizeof window);
    CHECK(p4_fn_h2g((void *)fn_c) == 0);
    CHECK(next_handle == HANDLE_BASE + 0x200u);
    CHECK(!p4_fn_known(next_handle));

    next_handle = P4_GUEST_EEREGS - 4u;
    CHECK(p4_fn_h2g((void *)fn_c) == P4_GUEST_EEREGS - 4u);
    CHECK(next_handle == P4_GUEST_EEREGS);
    CHECK(p4_fn_h2g_sig((void *)fn_d, "v") == 0);
    CHECK(next_handle == P4_GUEST_EEREGS);
    CHECK(!p4_fn_known(P4_GUEST_EEREGS));
    CHECK(p4_fn_h2g((void *)fn_a) == first);

    next_handle = 0xfffffffcU;
    CHECK(p4_fn_h2g((void *)fn_d) == 0);
    CHECK(next_handle == 0xfffffffcU);
    next_handle = 0x0050d348u;
    CHECK(p4_fn_h2g((void *)fn_d) == 0);
    CHECK(next_handle == 0x0050d348u);
    CHECK(!p4_fn_known(0x0050d348u));
    puts("guest function handle boundaries passed");
    return 0;
}
