/* Loads the retail SLUS_217.82 image at its PS2 address, redirects function
 * pointers stored in its data to native code, and records trap calls.
 *
 * A trap stub the port has not replaced calls p4pc_trap. By default that
 * aborts. With P4PC_TRAP_CONTINUE=1 it logs the first call of each distinct
 * trap and returns 0, so the run goes on and the log lists the work left. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/tables.h"

static int image_loaded;
int p4pc_image_loaded(void) { return image_loaded; }

static void fail(const char *what) {
    fprintf(stderr, "p4pc: %s: %s\n", what, strerror(errno));
    exit(2);
}

static void fail_why(const char *what, const char *why) {
    fprintf(stderr, "p4pc: %s: %s\n", what, why);
    exit(2);
}

/* PS2 RAM as the game's allocators see it. kwlnBoot (kwlnProbeHeapBudget) puts the
 * boot heap just below 0x02000000, so the whole range is mapped, not only the
 * ELF's extent. The image is loaded into it. */
#define RAM_LO 0x00100000u
#define RAM_HI 0x02000000u

#ifdef P4_GUEST64
/* 64-bit host (docs/port64.md): guest RAM is the one block p4_guest_init allocates anywhere, and the EE register page is one of its windows. */
static void map_ram(void) {
    if (p4_guest_init() != 0) fail("allocate the guest block");
    fprintf(stderr, "p4pc: guest block 0x%08x bytes at %p\n", (unsigned)P4_GUEST_SIZE, (void *)p4_guest_base);
}

static void map_ee_regs(void) {}
#else
static void map_ram(void) {
    const char *why = NULL;
    if (p4_os_map_fixed(RAM_LO, RAM_HI - RAM_LO, &why) != 0)
        fail_why("map PS2 RAM 0x00100000..0x02000000", why);
    fprintf(stderr, "p4pc: mapped PS2 RAM 0x%08x..0x%08x\n", (unsigned)RAM_LO, (unsigned)RAM_HI);
}

/* The EE register page at 0x10000000 (timers, DMAC, IPU, VIF, INTC). The port has
 * no hardware behind it, so it is zeroed memory. The platform writes the timer
 * registers there, and the decomp reads back what it wrote. Other registers are
 * not modelled. */
static void map_ee_regs(void) {
    const char *why = NULL;
    if (p4_os_map_fixed(0x10000000u, 0x10000u, &why) != 0)
        fail_why("map EE registers 0x10000000..0x10010000", why);
    fprintf(stderr, "p4pc: mapped EE registers 0x10000000..0x10010000\n");
}
#endif

/* The 32-bit little-endian ELF structures, as the file stores them. */
typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} elf32_ehdr;

typedef struct {
    uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} elf32_phdr;

#define ELF_PT_LOAD 1
#define ELF_MACHINE_MIPS 8

static int read_at(FILE *fp, uint64_t off, void *dst, size_t n) {
    clearerr(fp);
    for (;;) {
        errno = 0;
        if (fseek(fp, (long)off, SEEK_SET) == 0) break;
        if (errno != EINTR) {
            if (errno == 0) errno = EIO;
            return -1;
        }
        clearerr(fp);
    }
    unsigned char *out = dst;
    while (n != 0) {
        errno = 0;
        size_t got = fread(out, 1, n, fp);
        out += got;
        n -= got;
        if (n == 0) return 0;
        if (ferror(fp)) {
            if (errno == EINTR) {
                clearerr(fp);
                continue;
            }
            if (errno == 0) errno = EIO;
            return -1;
        }
        if (feof(fp)) return -1;
        if (got == 0) {
            errno = EIO;
            return -1;
        }
    }
    return 0;
}

static void fail_read(FILE *fp, const char *what) {
    if (!ferror(fp) && feof(fp)) fail_why(what, "unexpected end of file");
    fail(what);
}

typedef struct {
    uint32_t name, type, flags, addr, offset, size, link, info, align, entry_size;
} elf32_shdr;
enum { IMAGE_SECTION_MAX = 256, IMAGE_PROGRAM_MAX = 128 };
static p4pc_image_guest_range_t image_ranges[IMAGE_SECTION_MAX];
static size_t image_range_count;

const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count) {
    if (count != NULL) *count = image_range_count;
    return image_range_count != 0 ? image_ranges : NULL;
}

/* Optional readonly evidence for mod write admission, not a second ELF loader.
 * A section must be allocated wholly inside an actually loaded segment.
 * Failure leaves the policy unavailable, without changing ordinary boot. */
static void capture_image_ranges(FILE *fp, const elf32_ehdr *eh,
                                 const p4pc_image_guest_range_t *segments,
                                 size_t segment_count) {
    size_t count = 0;
    image_range_count = 0;
    if (eh->e_phentsize != sizeof(elf32_phdr) ||
        eh->e_phnum > IMAGE_PROGRAM_MAX || eh->e_shnum == 0 ||
        eh->e_shnum > 4096 || eh->e_shentsize != sizeof(elf32_shdr))
        return;
    for (unsigned i = 0; i < eh->e_shnum; ++i) {
        elf32_shdr sh;
        if (read_at(fp, (uint64_t)eh->e_shoff + (uint64_t)i * sizeof sh,
                    &sh, sizeof sh) != 0)
            return;
        if (!(sh.flags & 2u) || sh.size == 0) continue; /* SHF_ALLOC */
        uint64_t end = (uint64_t)sh.addr + sh.size;
        if (sh.addr < RAM_LO || end > RAM_HI || count == IMAGE_SECTION_MAX)
            return;
        size_t segment = 0;
        for (; segment < segment_count; ++segment)
            if (sh.addr >= segments[segment].begin && end <= segments[segment].end)
                break;
        if (segment == segment_count) return;
        unsigned flags = (sh.flags & 1u) ? P4_IMAGE_GUEST_WRITE : 0;
        if (sh.flags & 4u) flags |= P4_IMAGE_GUEST_EXEC; /* SHF_EXECINSTR */
        image_ranges[count++] = (p4pc_image_guest_range_t){sh.addr, (uint32_t)end, flags};
    }
    image_range_count = count;
}

/* Loads every PT_LOAD segment at its link address inside the mapped RAM.
 * The mapping supplies zeroed bss. */
uint32_t p4pc_load_image(const char *path) {
    image_range_count = 0;
    image_loaded = 0;
    FILE *fp = p4_os_fopen_rb(path);
    if (fp == NULL) fail("open image");

    elf32_ehdr eh;
    if (read_at(fp, 0, &eh, sizeof eh) != 0) fail_read(fp, "read ELF header");
    if (memcmp(eh.e_ident, "\x7f" "ELF", 4) != 0 || eh.e_ident[4] != 1 /* ELFCLASS32 */ ||
        eh.e_ident[5] != 1 /* ELFDATA2LSB */ || eh.e_machine != ELF_MACHINE_MIPS) {
        fprintf(stderr, "p4pc: %s is not a little-endian MIPS32 ELF\n", path);
        exit(2);
    }
    map_ram();
    map_ee_regs();

    /* The loader already owns the process-wide image_ranges; retain its bounded
     * segment scratch alongside that state rather than reinitializing a large
     * stack buffer during each boot. Only entries below segment_count are read. */
    static p4pc_image_guest_range_t segments[IMAGE_PROGRAM_MAX];
    size_t segment_count = 0;
    for (unsigned i = 0; i < eh.e_phnum; i++) {
        elf32_phdr ph;
        if (read_at(fp, (uint64_t)eh.e_phoff + (uint64_t)i * sizeof ph, &ph, sizeof ph) != 0)
            fail_read(fp, "read program header");
        if (ph.p_type != ELF_PT_LOAD || ph.p_memsz == 0) continue;

        if (ph.p_vaddr < RAM_LO || (uint64_t)ph.p_vaddr + ph.p_memsz > RAM_HI) {
            fprintf(stderr, "p4pc: PT_LOAD 0x%08x..0x%08x is outside PS2 RAM\n",
                    (unsigned)ph.p_vaddr, (unsigned)(ph.p_vaddr + ph.p_memsz));
            exit(2);
        }

        if (ph.p_filesz != 0 && read_at(fp, ph.p_offset, P4_G2H(ph.p_vaddr), ph.p_filesz) != 0)
            fail_read(fp, "read PT_LOAD");
        if (segment_count < IMAGE_PROGRAM_MAX)
            segments[segment_count++] = (p4pc_image_guest_range_t){
                ph.p_vaddr, ph.p_vaddr + ph.p_memsz, 0};
        fprintf(stderr, "p4pc: loaded 0x%08x..0x%08x (file 0x%zx, bss 0x%zx)\n",
                (unsigned)ph.p_vaddr, (unsigned)(ph.p_vaddr + ph.p_memsz),
                (size_t)ph.p_filesz, (size_t)(ph.p_memsz - ph.p_filesz));
    }
    capture_image_ranges(fp, &eh, segments, segment_count);
    image_loaded = fclose(fp) == 0;
    return eh.e_entry;
}

/* 32-bit host: replaces each data word that holds a PS2 function address with the native function (or generated trap)
 * address. 64-bit host: patches nothing. The words keep their retail code addresses, and the function table (guest.h) maps each
 * retail address to the native or trap that stands for it: every compiled retail entry point (p4pc_native_table), and the
 * target of each function word in the image (p4pc_patch_table, keyed by the word's current value). */
size_t p4pc_apply_patches(void) {
#ifdef P4_GUEST64
    for (size_t i = 0; i < p4pc_native_count; i++)
        p4_fn_register_sig(p4pc_native_table[i].addr, p4pc_native_table[i].fn, p4pc_native_table[i].sig);
    for (size_t i = 0; i < p4pc_alias_count; i++) p4_fn_alias(p4pc_alias_table[i].addr, p4pc_alias_table[i].fn);
    for (size_t i = 0; i < p4pc_hostfn_count; i++) p4_fn_h2g_sig(p4pc_hostfn_table[i].fn, p4pc_hostfn_table[i].sig);
    for (size_t i = 0; i < p4pc_patch_count; i++) {
        uint32_t value = *(const uint32_t *)p4_g2h(p4pc_patch_table[i].addr);
        p4_fn_register_sig(value, p4pc_patch_table[i].target, p4pc_patch_table[i].sig);
    }
    return p4pc_patch_count;
#else
    for (size_t i = 0; i < p4pc_patch_count; i++) {
        uint32_t *word = (uint32_t *)(uintptr_t)p4pc_patch_table[i].addr;
        *word = (uint32_t)(uintptr_t)p4pc_patch_table[i].target;
    }
    return p4pc_patch_count;
#endif
}

#define MAX_TRAPS 4096

struct trap_rec {
    const char *name;
    uint32_t ps2_addr;
    unsigned calls;
};

static struct trap_rec trap_log[MAX_TRAPS];
static unsigned n_traps;     /* distinct trap names seen */
static unsigned total_traps; /* calls, repeats included */
static int trap_continue;

static void report_traps(void) {
    if (total_traps != 0)
        fprintf(stderr, "p4pc: %u distinct traps, %u calls\n", n_traps, total_traps);
}

void p4pc_trap_configure(void) {
    const char *cont = getenv("P4PC_TRAP_CONTINUE");
    p4_os_init();
    trap_continue = cont != NULL && strcmp(cont, "1") == 0;
    atexit(report_traps);
}

int (*p4pc_trap_hook)(const char *name, uint32_t ps2_addr, const uint32_t args[4]);

/* Trap stubs call this. The first call of each name is logged in order; repeats
 * are only counted. Names beyond MAX_TRAPS are logged on every call. */
int p4pc_trap_args(const char *name, uint32_t ps2_addr, void *caller, const uint32_t args[4]) {
    unsigned i = 0;
    int first = 1;
    if (p4pc_trap_hook) return p4pc_trap_hook(name, ps2_addr, args);
    total_traps++;
    while (i < n_traps && strcmp(trap_log[i].name, name) != 0) i++;
    if (i < n_traps) {
        trap_log[i].calls++;
        first = 0;
    } else if (n_traps < MAX_TRAPS) {
        trap_log[n_traps].name = name;
        trap_log[n_traps].ps2_addr = ps2_addr;
        trap_log[n_traps].calls = 1;
        n_traps++;
    }
    if (first)
        fprintf(stderr, "p4pc: TRAP %s (PS2 0x%08x) called from %p\n", name, (unsigned)ps2_addr, caller);
    if (!trap_continue) {
        fprintf(stderr, "p4pc: stopping at trap (P4PC_TRAP_CONTINUE=1 logs and continues)\n");
        abort();
    }
    return 0;
}

void p4pc_trap(const char *name, uint32_t ps2_addr, void *caller) {
    p4pc_trap_args(name, ps2_addr, caller, NULL);
}
