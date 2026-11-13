# The 64-bit port model

Baseline: Android, iOS, macOS, Linux and Windows ([pc_port.md](pc_port.md)), so arm64 and x86_64. Today's build is i386 on Linux and Windows and must keep working while this path is built beside it. This page has the inventory of what assumes a 32-bit host, the model and the mechanism that was chosen with the evidence for it, the prototype that runs it, the staging plan and the cost. Everything numeric was produced by the tools under `tools/port64/` (reproduce with "Reproducing the numbers" at the end); estimates are marked **[estimate]**.

## Decisions

| Question | Decision |
|---|---|
| Where does guest memory live? | One allocated block (`p4_guest_base`, 64 MB, mapped by `p4_os_map_anywhere`), no fixed address. Host pointer = base + guest address; guest address 0 is NULL; 0xFFFFFF00.. are pointer sentinels (`(T *)-1`). |
| What is a pointer inside guest memory? | A 4-byte guest address (`u32`), as on the PS2, in the retail image, in saves and asset streams. Struct layouts stay PS2-shaped and are asserted. |
| What is a pointer in a register (local, parameter, return value)? | A host pointer. Pointer arithmetic, comparison, `->` and calls stay native C. Conversion happens only where a pointer enters or leaves guest storage, and at every integer/pointer conversion. |
| How is the decomp's C changed? | By a clang-AST rewrite of each **preprocessed** unit (`tools/port64/gptr_rewrite.py`, driven per unit by `unit64.py`; CMake target `p4decomp64`): macros are plain text by then, so nothing is edited inside a macro body. The decomp is not edited, overrides are written once for both word sizes. |
| Function pointers in guest memory | Guest code addresses. `P4_FN_G2H` looks the address up in a function table (retail address to native function or trap stub); a function without a retail address gets a handle on first store. |
| Calling convention at a call | The callee's definition decides which arguments are host pointers and which are guest addresses; a call whose declaration disagrees is adapted (2.6). |
| What lives outside the block | Windows the guest can address: the executable's own image at `0x04000000` (so string literals, port statics and functions need no copy), the EE register page at `0x10000000`, the scratchpad at `0x70000000`. |
| Stacks | Live in guest memory on every platform, so the address of a local is a valid guest address. Needs the thread model's coroutines to switch stacks themselves. |
| Host-only objects (SDL, GL, FFmpeg, host heap) | Never referenced from guest memory except through handles. A `P4_GUEST_CHECK` build faults on the first host pointer stored into guest memory. |
| Layout | Every record that holds a pointer gets generated `_Static_assert`s of its i386 layout (size and every field offset). |
| Port-owned code from now on | Does not assume `sizeof(void *) == 4`; new seams take `u32` guest addresses (CONTRIBUTING.md, "64-bit from now on"). |

## 1. Inventory

### 1.1 Method

`tools/port64/inventory.py <i386 build dir> <out>` compiles every unit of an existing i386 build tree twice, for `--target=i386-linux-gnu -m32 -malign-double` (what ships) and for `--target=x86_64-linux-gnu` (same mirror, same flags otherwise), with the pointer warnings on, and dumps the record layouts of both (`-S -emit-llvm -O0 -Xclang -fdump-record-layouts`; `-complete` ignores a typedef's `aligned(16)`, so `struct Model` is 720 bytes on i386 and not what it prints). It also does the port-owned `src/`. `tools/port64/census.py` then runs the rewriter (below) over every preprocessed unit and counts how often each rule fires: it reads clang's own AST, so it sees implicit conversions that no warning reports. Both run in about a minute on 24 cores. Mirror: 1231 units compiled from 1415 files, 847k lines of decomp C, plus 567 override files (38k lines) that are spliced into those units; port-owned `src/`: 105 files, 34k lines.

### 1.2 What a 64-bit compiler says about the decomp mirror

| Diagnostic (x86_64) | Sites | What it is |
|---|---|---|
| `-Wint-to-pointer-cast` | 6657 | m2c's `(T *)(a0 + off)`: a guest address held in an `s32`/`u32` used as a pointer |
| `-Wpointer-to-int-cast` | 3828 | `(u32)ptr`, `(s32)ptr` |
| `-Wint-to-void-pointer-cast` / `-Wvoid-pointer-to-int-cast` | 1208 / 492 | the same through `void *` |
| `-Wint-conversion` | 766 | implicit pointer/integer conversion: an argument or assignment, no cast |
| `-Wincompatible-pointer-types` | 401 | already warned on i386 (not word size) |
| other (`-Wcast-function-type-strict` 162, `-Wshorten-64-to-32` 150, 37 errors, ...) | 472 | |
| **Total (deduplicated sites, a header counts once)** | **13824** | |

- 40 of 1231 units do not compile for x86_64 as they are. 39 are the decomp's own size checks (`typedef char X[sizeof(struct S) == N ? 1 : -1]`, which already assert struct sizes and fail on LP64); one is a pointer-valued static initializer.
- `decomp/include/type.h` typedefs `size_t`, `intptr_t` and `uintptr_t` as 32-bit integers and `include/rw/std/stddef.h` repeats `size_t`; both clash with the host's on LP64. Four struct fields hold one of the word types (`WindowWork` has a `size_t`; one `long` field in 2408 layouts); the rewriter makes those `u32`/`s32`, and on a 64-bit host `p4pc/compat.h` pre-empts both headers so every other use is the host type.
- Records (distinct layouts by tag or definition site): 2408; 628 hold a pointer field (1750 pointer fields, 209 of them function pointers); **687 change size or a field offset** on x86_64. These are the structs the retail data, saves and RenderWare streams lay out with 4-byte pointers.
- Loader: 2398 data words in the retail image hold function addresses and are patched to host function pointers at load (2292 native, 106 trap); 4638 absolute data symbols are linker-defined at retail addresses (3761 distinct `D_xxxxxxxx` names and 926 gp-relative `iGp...` names in the source; only 3 names sit at or above 0x02000000).

### 1.3 What the AST sees (rewriter census, 1231 units)

Sites the rewriter changes in the preprocessed units (a header site counts once), 94,024 in all:

| Site kind | Sites |
|---|---|
| Load of a pointer from guest storage (field, array element, `*(T **)(p + off)`, absolute data) | 24779 (+ 3659 of function pointers) |
| Casts whose target is a pointer to a pointer (`*(u8 **)(p + 0x30)`: m2c's field idiom) | 15100 |
| Reference to retail-addressed data (`D_`, `iGp`, symbol list) | 22893 (11518 arrays, 6009 scalars, 3428 pointers, 1930 pointer arrays, 8 of an anonymous type) |
| Explicit integer to pointer casts / pointer to integer casts | 8540 (+ 498 function pointer, + 429 implicit) / 4300 (+ 92 function, + 304 implicit) |
| Declarations that become guest storage (`u32`) | 6211 (3671 variables, 2540 fields) |
| Pointer-to-pointer locals, parameters and results (`T **` becomes `u32 *`) | 746 |
| Store of a pointer into guest storage | 3039 (+ 507 function pointers, 572 slot-to-slot copies that need no conversion, 583 NULL) |
| Assignment whose value is used, `p++` / `p += n` on a guest-stored pointer | 492 / 68 |
| Records with a pointer field (get layout asserts) | 856 |
| `sizeof` of a pointer type | 62 |
| Static initializers that hold an address or a string literal (become init functions) | 38 (+ 17 string literals stored into guest memory) |
| Taking the address of a local variable / decaying a local array | 6654 / 12002 |
| Functions whose address is used as a value (need a function-table entry) | 1494 |
| Calls adapted to their callee's definition (2.6): arguments / results | 2925 calls (2043 arguments, 1029 results) |

Other greps over the mirror: 22 `va_start` sites in 21 units; 840 inline-asm lines; 103 `static` locals; 133 `intptr_t`/`uintptr_t` uses; 1 use of `alloca`/`setjmp`/`longjmp`.

Read this as: the 32-bit assumption is almost entirely in **casts and field idioms, not in declared types**: m2c keeps most pointers in integers and reaches fields through `*(T **)(p + off)`. Declarations (6957 of the roughly 94,000 sites) are under a tenth of it, so a fix that only retypes declared pointers leaves more than 90% of the sites. The call ABI row is the one a compiler cannot see: 2925 calls pass an integer where the definition takes a pointer, or the other way round, and every one is silent on a 64-bit host.

### 1.4 Port-owned code

- `tools/mipsdiff/sigs.txt` lists 1311 override and port-native functions, 1016 of which take or return a pointer: the seams. Their callers are the decomp, their callees are host code; today both sides share the 4-byte pointer.
- Historical pre-port `src/` probe: 105 files compiled for x86_64: 17 do not compile as they are, 15 of them on layout assertions the port already carries (`P4RwDevice`, `P4RwRaster`, `P4SjUni`: `p4gl.h` is included by 13 render and viewer files) and 2 on host headers outside the probe (`windows.h`, dlmalloc's include path). 97 port records mirror game structs; 51 hold a pointer and 58 change size. 682 `uintptr_t`/`intptr_t` mentions in `src/`, `include/` and `tools/`, 16 `va_list` lines. The pointer warnings (71) understate this code: its pointers are mostly `void *` that are dereferenced as guest memory, which no warning flags.
- Kernel model: threads are coroutines (`p4_coro_*`, `include/p4pc/os.h`): `ucontext` on Linux running on the guest's stack memory, fibers with their own stacks on Windows, and on the 32-bit Android build `src/platform/os/coro_asm.c`, the i386 context switch ([android.md](android.md)). The heap (`kernel_heap.c` / `ee_heap.c`) is the literal retail EE newlib implementation over the boot arena: chunk/bin words remain four bytes, with original alignment, coalescing and reuse; RW retains its matched checked allocation layer. Host scratch remains host-allocated. See the [CRT boundary](platform/crt_boundary.md). Boot maps RAM with `p4_os_map_fixed` (`MAP_FIXED_NOREPLACE` on Linux, a `.ps2ram` PE section on Windows).
- `config/port_owned.txt` names 27 data symbols with no retail address (RenderWare freelists and statics); they need guest addresses in the port half of the block.
- Verification tools: `tools/mipsdiff/r5900.c` reaches memory through one function (`mp()`), `ovdiff.c` tracks writes with `mprotect` on identity-mapped pages and calls natives through an i386 cdecl thunk, `replay.c` loads recorded pages at their addresses.

### 1.5 What is not a problem

Endianness (little on every target). Struct fields of `size_t`, `intptr_t` or `uintptr_t` (four word-type fields, all handled). Float formats. Pointer fields that point at data only host code reads (they never exist in guest memory). `alloca`/`setjmp` (one use).

## 2. Design

### 2.1 Guest address space

`include/p4pc/guest.h` (included by `p4pc/compat.h`, so every decomp unit and port file sees it):

- `P4_G2H(a)`: `a ? p4_guest_base + a : NULL`. `P4_H2G(p)`: the reverse. On i386 both are plain casts, so port code written with them builds for both.
- The block is 64 MB: 32 MB of PS2 RAM (0..0x01FFFFFF) and 32 MB the port owns (data with no retail address, a string pool, handles). The loader copies the retail image into it as the i386 build does; nothing executes the code bytes there (the decomp's C is compiled natively), the interpreter only reads them.
- 0xFFFFFF00..0xFFFFFFFF are sentinels: the decomp compares pointers with `(T *)-1`; `P4_G2H` maps them to a pointer that is never valid and `P4_H2G` maps it back, so the comparison and a stored sentinel round-trip. `P4_GUEST_CHECK` makes any other address outside the block fault in `P4_G2H` / `P4_H2G`.
- **Windows outside the block** (`p4_guest_window`, `include/p4pc/guest.h`): the fast path of `P4_G2H` is the block check; anything else takes a slow path through at most eight windows. `p4_guest_init` adds the executable's own image at `P4_GUEST_IMAGE` (0x04000000; its range comes from `p4_os_image_range`: `__ehdr_start`..`_end` on Linux and Android, `__ImageBase` on Windows), the EE register page (`0x10000000`, 64 KB, host memory) and the scratchpad (`0x70000000`, 16 KB). The image window is what lets a string literal, a port-owned static or a function be stored into guest memory with `P4_H2G` and no copy or pool, and what lets `static T *x = &y;` be an ordinary run-time store. GS, DMA and VIF stay replaced at the seam (CONTRIBUTING.md).
- Apple: a 64-bit process there cannot map below 4 GB (the kernel reserves a 4 GB `__PAGEZERO`; linking with less does not help on arm64), so a fixed low address (the i386 approach, or Linux's `-mx32`) is not available on three of the five platforms. The block is placed by the OS anywhere, at a multiple of 4 GB when the address space has such a range (`p4_os_reserve_aligned`, tried first; the plain reservation is the fallback): the low 32 bits of a host pointer into the block then equal its guest address, and ASLR only moves the upper half. The existing reservation/window policy is retained. Historically it also stabilized the former dlmalloc backend's eight-byte native `fd`/`bk` and tree links left in freed memory: the thunder-bolt array reads fresh `+0x18` state before writing it, and `(state & 0xFF000000) > 0x40000000` selected a different `effMiscRand`/camera path in 3 of 8 `castle_training_0421` runs with arbitrary placement (`setarch -R` matched); aligned placement matched i386 in 8 of 8 runs and the then-current 29 story gates. That is historical evidence, not the current allocator model: literal EE newlib now stores four-byte **guest** chunk/bin links and the matched RW checked headers. Host addresses no longer enter those metadata words; complete retail heap and semantic-route comparisons are documented in [the CRT boundary](platform/crt_boundary.md).

### 2.2 Where a pointer lives

| Where | Representation | Why |
|---|---|---|
| Struct or union field, global, `static`, array element, address-taken local | `u32` guest address | Must match the PS2 layout; the retail data holds these |
| Local, parameter, return value, temporary | host pointer | Keeps arithmetic, comparisons, `->` and calls native and fast |
| Pointer to a pointer (`T **`) | `u32 *` (host pointer to a guest slot) | The slot is guest storage |
| Function pointer in guest storage | guest code address | See 2.5 |

A conversion is therefore needed (a) when a pointer is loaded from or stored to guest storage, (b) at every integer/pointer conversion (a cast or an implicit conversion): an integer that stands for a pointer *is* a guest address, which is what m2c code means by it.

### 2.3 Mechanism, and the evidence for it

The requirement is a transformation that sees types: whether `x->next` is a pointer load or an integer load, whether `(u32)p` converts a pointer, and what the pointee type is for the cast, can only be known from the AST. The options that were evaluated:

| Option | Result | Verdict |
|---|---|---|
| **A. A `gen_port` text pass** (regex, like the existing mirror passes) that retypes pointer fields and rewrites dereferences | The existing passes rewrite declarations and calls by name. This one has to decide, for 24779 loads, 8540 casts and 15100 `(T **)` casts, what each expression's type is: `*(u8 **)(p + 0x30)` and `*(u8 *)(p + 0x30)` differ only in a type the text does not give, and macros (`RWSRCGLOBAL(x)`, `SJ_Reset(sj)`) hide the rest. | Rejected: it would need a C parser with types, which is the next option |
| **B. clang address spaces / `__ptr32`** | `struct S { int * __ptr32 p; }` is 4 bytes on every x86_64 target and `*p` compiles to `movslq; movl`: a sign-extension, no base added. On every AArch64 target tested (aarch64-linux-gnu, aarch64-linux-android, arm64-apple-macos, arm64-apple-ios, aarch64-pc-windows-msvc) `sizeof` stays 16 (clang 19). `address_space(N)` on a field leaves it 8 bytes on all of them. | Rejected: only x86_64, and it cannot add the base, so it still needs the fixed low mapping that Apple forbids |
| **C. C++ and a `gp<T>` class with operator overloads** | 55 of 120 sampled units compile as C++ with `-fpermissive`; the 666 errors are the same integer/pointer casts plus 184 K&R calls. Overloads would remove the loads and stores (about a third of the sites) but not the 8540 + 4300 explicit casts, which an operator cannot intercept, and every field declaration still has to be rewritten. | Rejected: the AST rewrite is needed anyway and covers all of it |
| **D. Retype every pointer to `u32` and translate every dereference** (what a wasm backend does) | Correct and uniform, but every `*p`, `p->f`, `p[i]` and `p + n` (scaled) needs rewriting, a much larger set than C's loads/stores/casts, and every pointer parameter changes ABI. | Rejected for the cost; the chosen model keeps registers native |
| **E. Compile the decomp to wasm32 and translate with wasm2c** | Not built. It would solve layout, function tables and stacks at once, but the port's value is in the existing link scheme (symbols at retail addresses, overrides spliced by address, `p4pc_native_table`, ovdiff and replay calling natives by address, 567 overrides and 105 port files calling into host code), all of which would be rebuilt around imports and exports, and it forks the build instead of running beside i386. | Kept as the fallback; M1 did not need it |
| **F. clang AST rewrite of the preprocessed unit (chosen)** | `tools/port64/gptr_rewrite.py` preprocesses a unit (`clang -E`, line markers kept), reads `clang -Xclang -ast-dump=json` of that text, which exposes every implicit cast (`LValueToRValue`, `IntegralToPointer`, `PointerToIntegral`, `NullToPointer`, `ArrayToPointerDecay`, `FunctionToPointerDecay`), applies the rules in 2.4 as edits to the text, and the result compiles with `-x cpp-output`. Line markers tell which edits are in the decomp's own text (port headers such as `p4pc/*.h` are never rewritten). Needs only the `clang` the build already uses and Python. | **Chosen. All 1231 units rewrite and compile for x86_64 with zero pointer/integer diagnostics and every layout assertion passing.** |

**Why preprocessed text.** The first version edited the source files. A node inside a macro body (`RwFree(p)` reads a field inside the macro, `SJ_Reset(sj)` reads `(sj)->vtbl`, `RWFRAMEGLOBAL(frames) = x` assigns a macro lvalue) can be edited only in the macro, and one macro body serves expansions of different types: editing bodies globally was tried and raised the residual from 1893 to 5816 diagnostics. On the preprocessed text every expansion is its own text with its own types, so the macro problem and the header-merge problem (a header seen by 400 units must be rewritten the same way in each) do not exist, and no macro layer is needed.

**Result** (`tools/port64/unit64.py report`, the ctest `port64_mirror`): 1231 of 1231 units compile for x86_64 with `-Werror` on `-Wint-to-pointer-cast`, `-Wpointer-to-int-cast`, `-Wint-conversion` and the void-pointer forms; no construct is left unsupported (`config/port64_unsupported.txt` is the baseline a regression is measured against). The remaining warnings (`incompatible-pointer-types` 398, `implicit-function-declaration` 285, ...) are the ones the i386 build already has.

Mechanisms added to reach that, beyond the rules of 2.4:

- **Word types.** `size_t`, `long`, `intptr_t` fields, `(size_t *)` casts, `sizeof` results kept in fields and absolute data of those types become `u32`/`s32` (4 field sites).
- **Read-modify-write.** `p++`, `--`, `+=`, `-=` on a pointer in guest storage (68 sites, including `T **` stepping by 4) are statement expressions that load, step by `sizeof(T)` and store back.
- **Pointer-to-array types** (`f32 (*)[16]`, `u16 (*)[311]`) and nested dimensions keep their element type.
- **Static initializers** that hold an address (38 sites) move into an init function: file scope into a generated `p4_ginit_N` registered by a constructor through `p4_guest_on_init`, function-local statics into a one-shot block after the declaration. The initializer text is extracted unchanged and applied with a `memcpy` or a compound literal, once the block exists. A string literal needs nothing more: the image window makes its address a valid guest address.
- **Address-taken pointer parameters** (`&param`) are renamed and given a guest-storage local at the top of the body; variadic functions are exempt (`va_start` names the last parameter).
- **Assignment used as a value**, chained assignments, **declarator groups** (`T *a, *b[4], c;`) and parenthesised declarators (`void *(**p)`).
- **ABI adaptation** (2.6).

### 2.4 The rewrite rules

All rules work on the original types from the AST. `T` is the type clang prints for the expression; `[...]` is the original text. The emitted code calls the inline functions `p4_g2h`, `p4_h2g`, `p4_fn_g2h`, `p4_fn_h2g` (the `P4_G2H`... macros of port code name the same functions).

| Original | Rewritten | Notes |
|---|---|---|
| `T *next;` in a struct, `T *g;` at file scope, `T *a[4]` | `u32 next;`, `u32 g;`, `u32 a[4]` | also a function pointer field, `void (*cb)(int)` becomes `u32 cb`; `T **p` local or parameter becomes `u32 *p`; a declarator group `T *a, *b;` is rewritten as a group |
| load of such an lvalue | `((T *)p4_g2h((u32)([lvalue])))` | function pointers: `p4_fn_g2h` |
| `lv = v;` with `v` a pointer | `lv = p4_h2g(([v]))` | a copy slot to slot, an integer made into a pointer only to be stored, and 0/NULL need no conversion; function pointers: `p4_fn_h2g` |
| `(T *)expr` with `expr` an integer; the same without a cast | `(T *)p4_g2h((u32)([expr]))` | `(u8 **)expr` also changes the cast type to `(u32 *)` |
| `(u32)ptr`, `(s32)ptr`, an implicit pointer-to-integer conversion | `(u32)p4_h2g(([ptr]))`; folded away when `ptr` is itself a load from guest storage | |
| `(s32)&((T *)0)->f` (the decomp's `MEMBER_OFFSET`, RenderWare's `rwLLLinkGetData`): an address computed from a null pointer constant through members, subscripts, no-op casts or `+`/`-` | `(s32)(unsigned long long)([expr])` | a byte offset into `T`, the same number on both sides since the rewritten layout is the i386 one; never `p4_h2g`, and `ptr + MEMBER_OFFSET(...)` is host pointer arithmetic, not a guest sum. Census rule `ptr2int-null-offset` |
| `D_00887300`, `iGpffffba04` | `(*(T *)p4_g2h(0x00887300u))`, arrays `(*(T (*)[N])p4_g2h(...))` | retail-addressed data; pointer-typed data is `u32` |
| `p++`, `p += n` on a guest-stored pointer | a statement expression: load, step by `n * sizeof(T)`, store | 2.3 |
| `sizeof(T *)` | `4` | |
| `R (*(*p)[N])(P)`, or `Slot *p` with `typedef R (*Slot[N])(P)`: a pointer to a table of function pointers (the SDK's render-state setter slots at `D_00887300`) | `u32 (*p)[N]`, the cast `(u32 (*)[N])`, and every `(*p)[i](...)` reads the slot as `p4_fn_g2h_sig((u32)((*p)[i]), sig)` | the typedef form and the spelled-out form take the same path (the classifier resolves the alias) |
| a call whose declaration disagrees with the callee's definition | a private declaration of the callee with the definition's types, arguments and result converted | 2.6 |
| any record with a pointer field | `_Static_assert(sizeof(struct S) == N, ...)` and one per field offset | N and offsets come from clang's i386 layout of the same unit (`-malign-double`); tag redefinitions in different blocks pick the layout whose field names match |

Example (the prototype, `kwlnTaskUnlinkFromList`):

```c
/* i386, as the decomp has it */
if (iGpffffba14 == task) iGpffffba14 = task->listPrev;
prev = task->listPrev;
/* x86_64, after the rewrite */
if (((KwlnTask *)p4_g2h((u32)((*(u32 *)p4_g2h(0x00764B04u))))) == task) (*(u32 *)p4_g2h(0x00764B04u)) = task->listPrev;
prev = ((struct KwlnTask *)p4_g2h((u32)(task->listPrev)));
```

The first `task->listPrev` is a slot-to-slot copy, so it stays a `u32` assignment.

**The null-offset rule.** The decomp's offsetof was first converted like any other pointer, `p4_h2g(&((T *)0)->f)`, and `p4_h2g_slow` gave the
number back only below 64 KB (a "pointer" under the first page). A field past 64 KB faulted with `host pointer outside guest memory and every window
stored as a guest address (0x1f2fc)`, or, without `P4_GUEST_CHECK`, read a wrong address: Shuffle Time pattern 3 (`btlShuffleSeq4Shuffle` 0037d840 and
the free-card pick inlined in `btlShuffleSeq4Run` 0037da60, `BtlShuffleContext.pattern` +0x1F2FC) and pattern 4's tutorial (`btlShuffleSeq5Run`
0037f6e0 phase 17, `flags` +0x1F2F4) on every 64-bit build, while i386 (identity conversions) was unaffected. The mirror has 2670 such conversions in
194 units, 2555 of them added to a pointer (formerly guest sums); 15 in 4 units name an offset of 64 KB or more: the three Shuffle Time sites,
`filerDrawDirectory` (00467880, the SDK file browser, `FilerTable.entryCount` +0x80F86) and 11 in `K_MapEditTask` (the map editor,
`FldMapEditorWork` +0x1CD10/+0x1CD14). All of them are integer conversions now, so no generated code relies on that band. The fixture
`tests/null_offset_rewrite_test.py` keeps an offset past 64 KB, and `shuffle_gate_*` ([testing](platform/testing.md#menu-and-battle-harnesses))
runs every Shuffle Time pattern and variant to its end.

### 2.5 Function pointers and callbacks

Retail data holds function words as retail code addresses (2398 of them). The i386 build patches them to host pointers at load; the 64-bit build does not patch anything:

- `p4_fn_register(addr, fn)` fills two open-addressing maps (retail address to host function, host function to address). The generated `p4pc_native_table` (every compiled function that stands for a retail entry point) and the `p4pc_patch_table` (the target of each function word in the image, keyed by the word's value) fill them at start-up; trap stubs are entries like any other.
- A function with no retail address gets a handle in `0x08000000..` the first time it is stored (`p4_fn_h2g`), except a function of the executable itself: it is its address in the image window (2.1), the same value `p4_h2g` gives it, so a function pointer that went through a data-pointer conversion (a `void *` parameter, a cast through `void *`) still comes back as the function.
- A value that is not in the table (`-1`, a retail address with no native, `1`) comes back from `p4_fn_g2h` as a tagged non-pointer (`0xDEAD000000000000 | value`) that never equals a function and round-trips through `p4_fn_h2g`. `kwlnTaskRunUpdate` stores the *return value* of an update callback into the task's update slot (`task->unk30 = (SdkTaskUpdate)result`): the callback returns the next function's address as an `s32`, so round-tripping arbitrary words is part of the contract.
- **Register signatures.** The same slot is called by m2c code that declares its pointers as `s32` and by typed code that declares them as pointers (the RenderWare allocator slots at `0x008873E8` are both), so the call site alone cannot say whether the callee takes a host pointer or a guest address. Each table entry therefore carries its function's register signature (`"pii"`: the result's class, then one letter per parameter: `p` a host pointer, `f` a function pointer, `i` an integer or guest address; from the ABI table for decomp functions and natives, `p4_fn_h2g_sig(f, "pii")` for a port function stored into guest memory), and each indirect call in the rewritten code passes its own (`p4_fn_g2h_sig(addr, 0x80000303u)`; also for a function pointer that is kept and called later: the signature comes from its declared type). When the two agree the function is called as it is; when they disagree on a pointer/integer pair `p4_fn_g2h_sig` returns one of 256 adapter entry points (`src/platform/guest.c`) bound to that (function, signature) pair, which converts the arguments and the result. Signatures with floats, structs or more than six parameters are not adapted.
- **Functions as values.** A named function that is stored into guest memory registers its own signature at the store (`p4_fn_h2g_sig(f, "pp")`: the one its definition has, from the ABI table or, for a static function of the unit, its own type; never the declared type of a function pointer variable or of an m2c placeholder `void f(void)`), so a table entry always carries the signature of the function behind it. A named function that is converted to a function-pointer type of another register ABI (passed to RenderWare as `RpMaterialCallBack(RpMaterial *, void *)` while m2c defines it `s32 f(s32, void *)`), or passed where the callee's ABI row says it calls its callback by another ABI (`fsig` in the row), goes through `p4_fn_adapt(f, own, declared)`: the function itself or its adapter. A call through a function pointer value that reached the unit as an integer uses `p4_fn_g2h_sig`.
- Port code that calls a callback it got as a guest address does `((cb_t)p4_fn_g2h_sig(addr, P4_SIG(n, r, a0, ..., a5)))(args)` with the typedef's own signature (guest.h; `P4_SIG(4, 0, 1, 1, 1, 0, 0, 0)` is an integer result and pointer, pointer, pointer, integer parameters), and stores its own functions with `p4_fn_h2g_sig`.

1494 functions are used as values; that is the callback table the loader fills from the generated tables, not a list to maintain by hand.

### 2.6 The call ABI

A register carries a host pointer when the callee's *definition* says pointer, and a guest address when it says `u32`/`s32`. Inside one unit the rewriter follows the prototype it sees. Across units that is not enough: the decomp's headers declared what m2c saw, and 2925 calls pass an integer where the definition takes a pointer or the reverse. That is silent on a 64-bit host, so the rewrite runs in two phases:

1. `unit64.py abi` records, per unit, the parameter and return classes (pointer, integer, other) of every non-static definition. `unit64.py merge --abi-files gen/obj64/abi-files.txt` joins CMake's explicit live game/native inputs into `gen/obj64/abi.json`: a name whose definitions disagree is dropped, and the `o` rows of `tools/mipsdiff/sigs.txt` add the port natives (their pointer parameters are host pointers, authoritative for a native). A removed unit's stale `.abi.json` is not an input, even in an incremental build. Native fixed-width `int64_t`/`uint64_t` entries retain `long long`/`unsigned long long` spellings rather than an LP64-only `long`; native `size_t`/pointer-width aliases remain host-width aliases when the shared metadata is compiled for LLP64.
2. A call whose declared classes differ from the definition's is adapted. The rewriter declares a private `extern RET p4abi_NAME_N(PROTO) __asm__("NAME")` at the current function position, renames the callee token, and wraps each argument and the result in `p4_g2h` / `p4_h2g` as needed. Using an asm-labelled declaration means no implicit-declaration, scope or K&R problem: the private name binds to the same symbol. The result type is the caller's own unless the definition's class has to replace it.

Native ABI files for generated build-tree sources are placed under
`gen/obj64/abi/generated/`; source-tree natives use `abi/port/`. This keeps
depfile paths inside their own output tree even on a cold parallel build.

A callee with no C definition (asm, trap stubs) keeps the declared types: a trap takes what the guest passed (guest addresses), which is what the retail code passes.

The table also holds the host library's functions by the prototypes `<string.h>`, `<stdio.h>`, `<stdlib.h>`, `<math.h>`, `<ctype.h>` and `<time.h>` give (the mirror calls `strcpy`, `memcpy` ... as m2c `s32` calls: `strcpy((s32)dst, (s32)src)` becomes a call with host pointers).

What the table covers: every non-static definition of the mirror units and of the port sources (`src/`: a native's ABI is its definition's), every other name the same retail function goes by (the m2c `func_003e1030` for the compiled `RwFreeListCreateAndPreallocateSpace`: the symbol lists, the `// FUN_xxxxxxxx` markers, `config/native_names.txt`, `#define Name func_xxxxxxxx`), the rows of `tools/mipsdiff/sigs.txt`, and variadic definitions whose `va_arg` reads are all integers (m2c's `va_arg(ap, s32)`: a pointer passed in the variable part is converted; printf-style definitions keep their variable part: only their fixed parameters are adapted, and a call through a variadic declaration stays variadic). A parameter that is a function pointer type is recorded as one (`p4_fn_g2h` / `p4_fn_h2g` convert it, not the data pair): a native that takes a callback must declare it as a function-pointer type, not `void *`. A call through another name for a symbol defined in the same unit (an asm label) is adapted like any other.

### 2.7 Stacks, host pointers, strings and statics

- **Stacks live in guest memory.** 17760 sites take the address of a local or decay a local array (5929 + 11831); most pass it to a callee that reads through it, some store it into a struct. With the stack inside the block every such address is a valid guest address and nothing needs a shadow stack. The thread model already creates its coroutines on guest stack memory on Linux; the main thread must run on one too (M2), and every platform needs a coroutine layer that does not depend on `ucontext` (Android and iOS lack it) or on the OS choosing the stack (Windows fibers): a small context switch per CPU, in assembly, as boost.context or libco do. The prototype's test runs on the process stack because the task unit never stores a local's address.
- **Host pointers never go into guest memory.** `P4_H2G` in a `P4_GUEST_CHECK` build faults on the first one, with the address. Known sources, and how each is handled: string literals and the address of a port-owned static or function (the image window, 2.1: no copy, no pool); port-owned storage the game addresses (`p4_gport_alloc`, the port half of the block); anything that holds an SDL/GL/FFmpeg object (a handle table; the renderer lane's seam).
- **Static initializers** that hold addresses (38 sites, in the CRI sources and RenderWare tables) cannot be C constants any more: the rewriter turns them into init functions registered with `p4_guest_on_init` and run by `p4_guest_init` once the block exists (2.3). No table step is involved.

### 2.8 Layout assertions

The i386 layouts are the oracle: clang computes them for the very unit being rewritten (an i386 `-S -emit-llvm -O0 -Xclang -fdump-record-layouts` of the same preprocessed text, `-malign-double`), and the rewriter inserts `_Static_assert(sizeof(...))` and one `__builtin_offsetof` per field after each record that holds a pointer. For `KwlnTask` that is 84 bytes and 18 offsets (the retail comments say `0x3C listNext`; the assertion says 60). The assertions also found rewriter bugs during the whole-mirror run (declarator groups left at 8 bytes, the typedef `aligned(16)` that `-complete` ignores), which is the argument for generating them rather than writing them. Every one of the 856 asserted records passes on the 1231 units. Port structs that mirror game structs carry their own asserts already (`ps2_fpbit.h`), and 15 `src/` files fail on them today on x86_64, which is the list of what to convert.

### 2.9 Rules for port-owned code (effective now)

Also in CONTRIBUTING.md. In short: no pointer/`uint32_t` casts or `uintptr_t` arithmetic on guest memory (use `P4_G2H`/`P4_H2G`); no port struct that mirrors a guest struct with a pointer member (`u32`, converted, asserted); never store a host pointer in guest memory; callbacks in guest memory are guest code addresses; new seams take and return guest memory as `u32` guest addresses and say so in their declaration, because the call sites follow the declaration. `include/p4pc/guest.h` is the only API they need and it is identity on i386, so the rule costs nothing today.

### 2.10 Kernel model, platform layer, float mode

- Coroutines: a per-CPU context switch on guest stacks (2.7); the Windows fiber backend and the `ucontext` one go.
- Boot: `p4_guest_init` (one `p4_os_map_anywhere`) replaces `p4_os_map_fixed` and the `.ps2ram` section on 64-bit; the loader copies PT_LOAD segments into the block and patches nothing.
- Float: the EE's round-toward-zero and flush-to-zero are MXCSR on x86 and FPCR (`RMode`, `FZ`) on arm64; `hostmath.h` and `p4pc_fpu_ee_mode` need an arm64 branch and the scene gates will show the difference.
- Unaligned and 128-bit accesses: x86 and arm64 tolerate unaligned scalar access; 128-bit `lq`/`sq` copies need the same `-mno-implicit-float` treatment they have on i386, and the interpreter already faults on misaligned `lq`.
- **AArch64 status** (measured, `P4_PORT64_TARGET="--target=aarch64-linux-gnu ..." ninja p4decomp64`, clang 19 cross, qemu-user for runs): the rewriter, the ABI table and the layout asserts are architecture independent (they come from the i386 oracle and from clang's AST); the AArch64 coroutine switch (`os_posix.c`: x19..x28, x29, x30, d8..d15 in a 176-byte frame; Mach-O spelling for Apple) passes `port64_coro_test` at -O0, -O1 and -O2 under qemu-user. What stops the mirror compiling for AArch64 today is exactly two things: `include/p4pc/eefloat.h` (every unit includes it; the EE's nearest-rounded division and square root are computed in the x87 `long double` with `"+t"` asm, and `long double` is a software quad on AArch64, so it needs an exact integer or `double`-pair implementation checked against the golden traces) and 6 sites that pass an old-style `va_list` as `void *` (a struct on AArch64). Everything else rewrites and compiles.
- `va_list` (22 sites): a `va_list` is a struct on x86_64 and arm64 Linux, a pointer on i386. The decomp's uses are debug-print paths bound to `vsprintf` in `ps2_libc.c`; they pass the host `va_list` through port code, never through guest memory.

### 2.11 Verification infrastructure on 64-bit

- The R5900 interpreter reads guest memory through `P4_G2H` now (`tools/mipsdiff/r5900.c`: three lines, identity on i386, ovdiff on i386 still runs 464 functions). Run over the x86_64 block it is the retail reference the prototype is checked against.
- `ovdiff.c` needs, for 64-bit: write tracking with `mprotect` on `p4_guest_base + page`, native calls through a per-signature C thunk instead of the i386 cdecl asm (the signature table `sigs.txt` already says which arguments are pointers: those are `P4_G2H`'d for a host-pointer callee), no `personality(ADDR_NO_RANDOMIZE)`. `replay.c`: pages loaded at `base + addr`.
- The scene gates (`title_mad`, `scene_mad`, `newgame_to_field`, battle, camp, save) compare against retail captures and need only the 64-bit executable.

### 2.12 The 64-bit executable (M2)

`cmake -DP4_PORT64_LINK=ON` (needs x86_64 SDL3 and FFmpeg: the p4pc-ci image has them) adds `p4pc64`:

- the mirror64 objects (`-DP4_PORT64_OPT=O2` builds them at the shipping optimization level; the default `O0` is the compile-only check). Both use the EE float rewrite when `P4_EE_FLOAT` is enabled, so an O0 float intrinsic cannot acquire a host `sqrtf` call. They link with `p4port64` and `p4portee64`, the sources of `p4port` and `p4portee` compiled for x86_64 with `P4_GUEST_CHECK` (a host pointer stored into guest memory faults);
- `p4pc_tables64.c` from `gen_port.py tables --ptr64`: the same native, patch and trap tables, the signature of each row from the ABI table, no absolute data symbols (retail data referenced by symbol from 64-bit code is a build error; port code uses `P4_GDATA`);
- start-up (`src/image.c`, `src/crt.c`): `p4_guest_init` reserves 4 GB of address space and commits the 64 MB block (base plus any 32-bit value is the block or an inaccessible page; a wild guest address faults on use, not on conversion), the PT_LOAD segments are copied into it, `p4pc_apply_patches` registers the function table (nothing is patched), and the game runs on a coroutine whose stack is 4 MB of the stack arena (`0x02000000..0x02FFFFFF`, `p4_guest_stack_alloc`), so the address of a local is a guest address;
- coroutines (`src/platform/os/os_posix.c`): on x86_64 the context switch is ours (a few instructions saving the callee-saved registers and the stack pointer, on exactly the stack memory it is given); `ucontext` stays for other architectures until M7;
- bring-up status: with the ISO, `p4pc64` runs the whole boot on x86_64 (devices, CDVD reads, GL ES, plugin registries, model/field loading) into the opening movie, and its console log is line-for-line the i386 build's up to there (compare the two runs on one host after normalising addresses); the faults it met were, in order, port-side ABI classes (Seams), the call/ABI cases above and the offsetof-by-NULL idiom. `P4_FN_TRACE=1` prints every indirect-call adapter it creates (call site signature, function signature, function address for `info symbol`);
- **Rules found by the bring-up** (each was a boot fault): (1) `p4_h2g` takes a host pointer up to 4 GB either side of the reservation modulo 2^32, because the 32-bit original wraps (`frames + (u32)offset` with a negative offset); (2) an adapter that escapes (a callback handed to a function that stores it) is stored as the function behind it (`p4_fn_h2g` maps an adapter back), so a table entry keeps the function's own signature; (3) a function word passed to a callee whose ABI row says it calls the argument by a callback signature (`fsig`) goes through `p4_fn_g2h_sig(word, sig)`, so a callback stored by the callee is called with the right register classes; (4) `printf`/`sprintf`/`snprintf`/`fprintf` from decomp code are routed to `p4pf_*` (`src/platform/ee_printf.c`) with an extra leading integer-class mask. Every masked integer expression is normalized to `unsigned long long`, preserving signed32 sign extension and unsigned32 zero extension; every set bit consumes one 64-bit GP word, even for `%d`, `%c`, guest pointers and starred fields on i386. Native pointers and real doubles remain unmasked in their native C classes. The shared retail formatter generates digits, not host `snprintf`; see [retail output formatting](porting.md#retail-output-formatting).
- debugging: `P4_PORT64_DEBUG=g ninja p4pc64` (after deleting the object of the unit of interest) compiles the 64-bit units with full debug info; the first failure of a run is a guest fault (`p4 guest fault: ...`) or a SIGSEGV whose address is `base + guest address`.

`P4_AUTO_INIT=pattern|zero` also reaches the custom `unit64.py build`
compilation of the rewritten mirror, through `--auto-init`. It is not enough
to initialize only the CMake-owned port objects: the FBN collision-proxy yaw
read lives in the mirror. The mode applies to the final native compile, not
the i386 layout oracle, and retains `-Wuninitialized` /
`-Wsometimes-uninitialized` diagnostics with their source lines for logic
comparison reports. The shipping default remains uninstrumented.

In the project's private CI, `P4PC_SCOPE=logic64-init sh tools/remote_check.sh logic64-study` runs the
logic guard at shipping `-O2`: pattern and zero builds are sequential,
including the rewritten mirror, not just the CMake-owned objects. It plays
New Game through 04/15, the 04/15 checkpoint through Castle 2F, and a real
two-leg checkpoint stretch (`story0417` -> freshly saved `story0418` ->
freshly saved `story0418tv`). Both modes start from the same private seed
cards; the second leg cannot fall back to an installed canonical next card.
Each saving leg must finish its walker and log `SDKMEMCARD_SAVE OK`.
Scripted input, the virtual clock and ordinary disc-load ordering are fixed.

Every 500-flip walk digest is compared, plus endpoint field/room/calendar,
event flags and BF indices/program counters/timers, all 624 MT words, unit transform words and FBN NPC
collision-proxy transform words. Addresses and matrix padding are not state.
Any differing word, missing digest, failed walk or absent snapshot fails;
there is no visual tolerance or quarantine in this scope. Reports list all
differences and source-line `-Wuninitialized` / `-Wsometimes-uninitialized`
candidates. Per-object `.uninitialized` files preserve mirror diagnostics
when an incremental build has nothing to compile. These are candidates,
not an assertion that every compiler warning caused the observed difference.

The full JSON report is named by `<build>/logic64-init/latest-report.txt`.
Failing cohorts retain their RAM, boot logs and cards; successful cohorts
retain the report (including semantic words and digests) and logs, then
remove bulky private RAM/card scratch. This compares same-source logic
between initialization modes; it does not claim retail frame lockstep.


### 2.13 Windows x64 (M6)

x86_64 Windows is LLP64: `long` is 4 bytes (as in the i386 original) and `size_t`/`uintptr_t` are 8. Measured with the llvm-mingw clang (`P4_PORT64_TARGET=--target=x86_64-w64-mingw32 unit64.py build`, `-O0`, `--check`): all 1231 mirror units rewrite, compile and pass their layout assertions. Two things differed from Linux and are fixed: (1) `include/p4pc/compat.h` pre-empts the decomp's `rw/std/stddef.h` shim before any host header (mingw's `<stdint.h>` includes `<stddef.h>`, which found the shim first: `typedef unsigned int size_t` against the host's 8-byte one); (2) the pointer-sized typedefs (`size_t`, `uintptr_t`, `ssize_t`, `ptrdiff_t`, `intptr_t`) are 32-bit words in a struct by name (`PTRSIZE_TYPEDEFS`), not by what the host resolves them to (`unsigned long long` here; `WindowWork.buffer_size` is a `size_t` member).

The CMake target `p4decomp64w` (option `P4_PORT64_WIN`, on when `CMAKE_C_COMPILER` can target `x86_64-w64-mingw32`, which is the CI image's llvm-mingw clang) is that check on every build, `-O0` (`P4_PORT64W_OPT` raises it); the ABI table (`abi.json`) is shared with the Linux variant because pointer/integer classes do not depend on the target.

**The executable.** With the image's `/opt/p4win64` (SDL3 and FFmpeg built by `ci/win32/build_deps.sh <prefix> <llvm-mingw> x86_64-w64-mingw32`) and `-DP4_PORT64_LINK=ON`, the same tree cross-builds `win64/p4pc64w.exe`: the mirror64 objects of `obj64w`, the port sources with `os_win32.c` for `os_posix.c` (`p4port64w`, `p4portee64w`), and `p4pc_tables64w.c` from `gen_port.py tables --ptr64 --coff64` (x86_64 COFF has no symbol prefix and one PC-relative relocation for calls and data alike, which is safe because a 64-bit mirror reaches no retail data by symbol). `P4PC_SCOPE=win64` (`tools/local_boot.sh --x64`) builds it with the mirror at `-O2` through the EE float rewrite. `ctest port64_coro_win` (the coroutine switch below) runs under 64-bit Wine.

**What Windows x64 needed beyond the compile.**
- The coroutine switch (`os_win32.c`): rbx, rbp, rdi, rsi, r12..r15, xmm6..xmm15 and the NT_TIB stack fields (StackBase, StackLimit, DeallocationStack) are switched with the stack pointer, on the stack memory the caller gives (the guest stack arena), so locals are guest addresses and exception dispatch accepts the stack. `tests/port64_coro_win_test.c` checks the registers, 100000 switches, a 64 KB frame (`__chkstk`), the TIB and an access violation reaching a vectored handler on a coroutine stack; it passes natively and under Wine.
- Stacks have a guard page below them and start on a page boundary (`p4_guest_stack_alloc`): `__chkstk` probes page by page from StackLimit down to the frame it is asked for and stops only on equality with a page, so an unaligned limit made it zero the whole guest block below an overflowing 4 KB thread stack. An overflow is now an access violation, and the fault note names it ("a coroutine stack overflowed into its guard page").
- A game thread whose own stack is smaller than 64 KB (the ADX server thread has 4 KB) runs on an owned default stack on both i386 and 64-bit hosts: native ABI, VFS, mod and mirror frames can need more stack than the PS2 code. This is not permission to use a host formatter; the [CRT boundary](platform/crt_boundary.md) binds formatting to the retail engine. `ReferThreadStatus` still reports what the game declared.
- A call whose declaration makes an argument 8 or 16 bits wide and whose definition makes it 32 (m2c `s16`, definition `s32`) is made through the definition's types (`abi-narrow-adapted`): clang's x86-64 System V callers extend narrow arguments, Windows x64 callers do not, and `calGetDayTableValue(s16, u8)` read garbage in the upper halves. The same for a narrow result read wide.
- Indirect-call adapters by position (`guest.c`): on Windows x64 a parameter is in rcx/rdx/r8/r9 or, as a float or double, in xmm0..xmm3 *of its own position* (the System V adapters count integer and xmm registers separately), and the fifth and sixth are on the stack. Each of the 1024 entry points is a stub in assembly (`p4_ad_common`) that saves the four integer and four xmm registers and the address of the stack arguments, calls `adapted_w64`, and returns {rax, xmm0}; the target is called by `p4_call_w64`, which loads them back. An adapter is never adapted again: `fn_for_sig` first looks through an adapter to the function behind it (a callback handed on twice made `ad0xEB` call `ad0xEA`).
- The crash handler (`os_win32.c`) is 64-bit: the registers, a first line that needs no C library (the heap may be what failed), the call stack by the PE unwind tables, and the guest address of an access (`p4_os_fault_note`).

**Rules found by running the New Game script on the 64-bit build** (`ctest newgame_to_field64`, field 7 room 2, 0.55% of the reference): (1) an integer-to-pointer cast of an arithmetic expression (`(u8 *)(i * 4)`, `(u8 *)(uintptr_t)(i * 0x2c)`, an offset added to a base afterwards) goes through `p4_g2h_off`, for which 0 is the block's base and not NULL: with `p4_g2h` the sum became a raw guest address read as a host address, and the exe image at its low addresses answered with code bytes (a field resource asked for the file `f26761_16644`); (2) a function pointer made by casting a data pointer word read from guest storage (`((FnVoidPtr)*(void **)(p + 0x318))(arg)`) converts the word with `p4_fn_g2h_sig`, not `p4_g2h`.

Boot status: with the ISO, `p4pc64w.exe` on the RTX 4080 runs the whole boot to the opening movie, the title and the logo loop with the same console log as the i386 build (no crash, no trap).

### 2.14 The full-game gates on the 64-bit build (M5)

Every test of `tests/CMakeLists.txt` that runs `p4pc` is also registered for `p4pc64` as `<name>64` with the same command line (a wrapper around `add_test`; labels `gl;port64`, so the land check's x64 shard runs them): the systems, story, save, battle, camp, fields, fusion and street gates therefore give their verdicts on both builds from one definition. The properties (timeout, skip code, working directory, environment) are copied to the clone. `scene_battle_floor_a` has no clone: its 64-bit twin (own limit) is registered by hand. `castle_2f_0417` and `home_0417` first faulted in `btlPacketStep`: `btlPacketAllocZeroed(void *callback, void *data)` got a function passed as `(s32)func`, which converted as a data address (`p4_g2h`), and the callee's `(BtlPacketCallback)arg0` stored a handle for that data pointer; a function passed as an integer to a pointer parameter now converts as a code address (`names_function`). `btlAICommandSelectCandidates` calls its `code arg5` parameter with a unit pointer where the callbacks (`btlCond_MYID`) take guest integers: an unprototyped function-pointer parameter takes its `fsig` from how the body calls it (`calls_through_params`). The clones of the long gates carry the label `slow` (minutes natively, hours under qemu-user: the a64 scope runs `-LE slow`).

A 64-bit build for gate verdicts is `-DP4_PORT64_OPT=O2`. The O0 compile-only mirror now also runs `tools/ee_float_cc.py` when `P4_EE_FLOAT` is enabled: optimization level is not an exception to the CRT boundary, and Win64 must not lower an unrewritten float sqrt to host `sqrtf`. Before that launcher was applied at O0, its host floats made the club walk collide with a wall 8 percent short, end elsewhere, and fail four gates on a frame that was not the game's. What the gates found, all in the 64-bit build only:

- **256 indirect-call adapters ran out** in Marukyu (`fn_for_sig`): the pool is 1024 (`P4_MAX_ADAPTERS`, both the System V C stubs and the Windows assembly stubs).
- **A three-byte buffer that the callee fills with four** (`FldUnit_PulseTintStep`: `u8 buf2[3]` for `mdlEffectGetPackedColor`) overwrote the saved `rbx` of its frame at -O2 and the task update that called it ran with a corrupt task (`config/mirror_fixes.txt`).
- **Void definitions whose callers use the value** (`evtCreateBlurFilterTask`, the battle packet and sound task starters, `datPersonaRunSkillSlot`, ...): at -O2 the compiler does not keep the callee's last result in `rax`. Each is a mirror fix that returns it (the existing tail-call pass covers direct calls; these end in an indirect call or a store). `P4_PORT64_VOIDUSE=<file>` lists the remaining such calls (101 names, most of them CRI forwarders whose callers ignore the value).
- **A by-value struct of floats (at most 8 bytes) is an integer parameter** (`tools/port64/gptr_rewrite.py`, `float_record`): System V and AAPCS64 pass `struct { f32 x, y; }` in floating-point registers and the m2c callers declare the same two floats as an `s64` in an integer register (i386 and Windows x64 pass both the same way). About 300 UI draw functions take one (`datPanelDrawByKind(Vec2f, f32, s32, u8 *, s32)`: the battle panel crashed the story gate at the first fight). The ABI row of such a definition has `sx` (class `int`, `unsigned long long`); the definition's parameter becomes `unsigned long long NAME_p4sx` with a struct local copied from it at the top of the body, a declaration of it is rewritten the same way, and a call that passes a struct of at most 8 bytes where the row says integer packs it (`__auto_type` and `memcpy`). Structs of more than 8 bytes keep their partial row.

Measured on x86_64 at 75e65ad (WSL, clang 19, `P4_PORT64_OPT=O2`, the CI retail directory): battle_menu, camp_menu, save_load, retail_save_load, fields_load, fusion, compendium, club (2.11 %), minimap (0.09 %), game1f (0.09 %), heaven1f (0.13 %), daidara, shiroku, story (field 7:2 on 04/15 at flip 140000), save_play (68:1) and walk_digest pass.

### 2.15 Performance of the 64-bit build

Measured (WSL2, clang 19, `-O2` mirror, offscreen SDL, llvmpipe, `P4PC_TURBO=2`; the story walk of `story_gate.sh` stopped at flip 40000 through `P4PC_NEWGAME_GATE`, user CPU seconds, two runs each; `perf record -e cpu-clock` for the profile):

| build | user s | note |
|---|---|---|
| i386 (`p4pc`) | 64.1 | the reference |
| x86_64 before | 86.4 | 1.34 x slower |
| x86_64 after | 54.0 | 16 % faster than i386 |

The same flow on the RTX 4080 (140000 flips, `tools/local_boot.sh`, hidden window): Win32 315 s, Windows x64 398 s before the change below (1.26 x) and 281 s after it (11 percent faster than Win32; one run each on a shared machine).

The whole difference was `fn_for_sig` (guest.c), which every indirect call through a function word runs: two linear scans of the 1024-entry adapter pool (is this pointer an adapter? is there already an adapter for this function and signature?) were 20 percent of the profile (plus `fn_find` 1.6, `adapted` 1.6). They are two open-addressing hash maps now (adapter entry point to index, (function, signature) to index). What is left of the translation cost in the profile is `fn_find` 1.9 percent, `adapted` 1.7, `fn_for_sig` 1.5, `p4_g2h_slow` 0.4: about 5 percent; the inline `P4_G2H` range check is not a symbol (it is in the callers). The rest of the profile is the same functions as on i386 in the same order (`p4_vu0_madd` 14 percent, `p4gl_atomic_draw` 11, `p4pc_ee_add` 8, `RwMatrixMultiply` 6, `p4pc_ee_sub` 4): the 64-bit build has more registers and the translation costs less than it saves. Candidates if more is wanted, none measured as needed: `p4pc_ee_add/sub/div` are calls into `ee_float.c` and could be inlined for the mirror (both builds), `p4_vu0_madd` is the VU0 emulation.

### 2.16 Cutover: 64-bit as the default build

This section records the change as it was made in the project's private CI. Its land-check, nightly, remote and
local runner scripts are not part of the public repository; the public build and tests are [BUILDING.md](../BUILDING.md)
and `.github/workflows/build.yml`.

**Decision (the user, 2026-10-20): 64-bit is the default product; i386 stays as the nightly reference build (not dropped).** Executed in one change (tools and CI only, no game code):

- **Land check** (`tools/land_check.sh`): the Windows x64 mirror, exes and viewer at -O2 (scope `x64w`, land host; it exports the exes, DLLs and the manifest of the `scenes64` gates as soon as its build ends), the Linux x86_64 mirror and its tests (scope `x64`, a third Linux host), and the game gates on the PC's GPU from the Windows x64 export (`tools/local_gates.py --abi x64`: the manifest of the tests labelled `scenes64`, whose `/lane/build/p4pc64` and `/lane/build/p4pc_viewer64` map to `p4pc64w.exe` and `p4pc_viewer64w.exe`). 39 gates: the twins of the 39 `scenes` gates, including the three viewer variants that had none (`title_mad_gl33`, `title_mad_ctxloss`, `field_mad_ctxloss`). A machine that cannot take the gates (PAUSE, no retail data, busy) hands them to the VPSes: all of them in the x64 shard (`P4PC_X64_GATES=1`) or the ones it left in scope `scenes64`. Tests that run no executable of the build (python and shell checks) take the label `port64`, so the x64 shard runs the ones the i386 `main` shard used to carry. Found by running the Windows gates for the first time: the x64w scope built the Windows mirror at -O0 (`P4_PORT64W_OPT` is separate from `P4_PORT64_OPT`): 8 of 36 gates failed (the club, minimap, game 1F and Heaven frames 12 to 22 percent off, rng_floor, walk_digest, castle_2f and home_0417); at -O2 all 39 pass, with `walk_digest64` equal to the 32-bit digest.
- **Nightly** (`tools/nightly_gates.sh`): the i386 reference (scope `main`: the 32-bit Linux build and tests, Win32 and its Wine tests), the i386 gates on llvmpipe (two hosts), the uninitialised-read guard, the 64-bit gates on llvmpipe (scope `scenes64`) and AArch64. A red night writes `<scratch>/nightly/RED` (failing jobs and tests); full logs live in managed `nightly/runs/<date>.<nonce>` trees, with a 32-GiB shared scratch budget and seven-day age limit. `tools/land_status.sh` shows RED and `tools/remote_land.sh` prints its banner, so every lane sees it at its next land. `P4PC_LAND_I386=1 sh tools/remote_land.sh <lane>` adds the i386 `main` shard to one land (a change to a 32-bit-only path should use it).
- **Nightly x64 logic-init guard:** the named `logic64-init` job runs `P4PC_SCOPE=logic64-init` in lane `nightly-logic64-init` on `P4PC_NIGHTLY_X64_HOST` (the same host setting as `x64-gates`; empty disables both). It compares pattern- and zero-initialized x64 builds through New Game to 04/15, castle 2F and a freshly saved 04/17 -> 04/18 -> TV-world chain, including every 500-flip digest and semantic RAM. It participates in ordinary nightly RED/history reporting, without quarantine; `P4PC_NIGHTLY_ONLY=logic64-init` selects just this guard.
- **Defaults**: `tools/local_boot.sh` runs `p4pc64w.exe` (`--x32`: the 32-bit exe), `tools/remote_boot.sh` builds and runs `p4pc64` (`--x32`: `p4pc`), `tools/win_digest.sh` runs the x64 exe (`P4PC_X32=1`), CONTRIBUTING.md names the 64-bit build as the default. `tools/package_win.sh` defaults to x64 (`--x32` for the reference zip; PortRelease).
- **Linux package** (`tools/package_linux.sh`, scope `pkg64`): `p4pc64`, SDL3 3.2.10 and an LGPL-only FFmpeg 7.1.1 built from source (`ci/win32/build_deps.sh <prefix> /usr x86_64-linux-gnu`; the distribution's FFmpeg is `--enable-gpl`) into the host's ccache directory, `tests/ffmpeg_license_test.c` run against the shipped libraries with `P4PC_REQUIRE_LGPL=1` (mandatory), a `p4pc` launcher that puts `lib/` ahead of the system's libraries, README and notices (`packaging/README_linux.txt`, `NOTICES_linux.txt`), one tar.gz of 8.7 MB. Measured: deps 244 s, p4pc64 -O2 1310 s on a cold lane on a third Linux host; unpacked in WSL, the launcher loads the shipped libraries (`ldd`), SDL3 lists wayland, x11, kmsdrm and offscreen video and alsa, pulseaudio and pipewire audio, and the New Game script reaches flip 6000 in field 100 (`-DP4_LINUX64_PREFIX=<dir>` is the CMake switch).
- **Fixed addresses**: untouched for the i386 reference (the `.ps2ram` section, `p4_ps2_exe()`, `MAP_FIXED_NOREPLACE`, the identity macros); CONTRIBUTING.md says new code must not depend on them.
- **Host tests**: every host test (an executable built from port sources, as opposed to the game) has a 64-bit twin run by the land check's x64 shard (docs/port64.md 6.8): 51 of the 52 tests that existed only as i386 executables (the CRI tests, `pad_*`, `vu0_helpers`, `ee_libm`, `kernel_threads`, `mixer_device_*`, `render_smoke_es30/gl33`, the retail override tests, ...); the 52nd, `memcard_roundtrip`, is the memcard model cohort's (PortSystems3).

Status (all measured on main at e72d017d unless marked): every test that runs `p4pc` has a `p4pc64` clone and the x64 shard passes (54 tests); Windows x64 runs the story walk to 04/15 on the RTX 4080 (398 s) and its digest equals the 32-bit one (`P4PC_X64=1 sh tools/win_digest.sh <lane>`; `walk_digest64` shares `tests/walk_digest_expected.txt`, so one file serves Linux and Windows, 32 and 64 bit); the Android arm64 build passes the phone gates (PortARM); AArch64 under qemu-user passes the standalone and replay tests (6.7).

Land-check time, before and after (the queue's `check=` seconds of single-submission lands): before the cutover (i386 `main` + `win` + the scenes shards + the x64 shard) warm 396-563 s, cold 1008-1078 s; after it, warm 671-847 s over five lands (671, 736, 783, 832, 847; median 783) and cold 965-1183 s. The warm cost went up by about four minutes because the Windows x64 mirror and exes are built at -O2 through the EE float rewrite on every land (about 6 min on a Linux host, the critical path: its export gates the game gates), where the Win32 build used to be a cached incremental one; the cold cost is the same. What it buys: the check builds and gates what ships (x86_64 Linux, Windows x64 on the GPU the game runs on), and i386 is caught within a day by the nightly. Batches of two or more submissions (`size=2` in the queue) cost about one check for both (1318 s for two).

What remains before 64-bit is the shipping build on Windows and Linux:

1. **Windows x64 package and smoke.** `tools/package_win.sh` and `tools/package_smoke.sh` (paced, story, load) on `p4pc64w.exe`, with the FFmpeg and SDL3 DLLs of `/opt/p4win64` and the license check of the FFmpeg build (PortRelease has it). Keep the Win32 zip for one release as the fallback.
2. **The game gates on the PC's GPU for x64.** The x64w shard runs the Windows host-test twins and the coroutine fixture under 64-bit Wine (`port64win`, excluding GL as in the Win32 scope). This does not replace the actual GPU export gates (`tools/local_gates.py --abi x64`): keep the same 64-bit export and nightly gate set, so a Windows-x64-only renderer regression cannot land.
3. **Linux x86_64 packaging.** Nothing builds a Linux package today (`package_win.sh` is the only packager) [inferred from the tools directory]; the decision is a tarball or an AppImage with SDL3 and FFmpeg.
4. **iOS is not built; macOS is** ([platform/macos.md](platform/macos.md)): arm64 and x86_64, natively on GitHub's macOS runners, with the Mach-O spelling in the tools (`--macho`), the GLES path on ANGLE's Metal back end and an ad-hoc signed app bundle. Still unverified there: everything that needs a real Mac (its list), and for iOS the code-signing and `pthread_jit` rules (6.7) and the port's own EGL on a `CAMetalLayer`. Linux and Windows do not wait for either.
5. **Open 64-bit items that the gates do not cover.** The `P4_PORT64_VOIDUSE` list (101 names whose result is used by a caller of a `void` definition: most are CRI forwarders; the ones the gates reached are mirror rows) and the definitions whose ABI row is still partial (`skip`: a struct of more than 8 bytes by value, struct returns); a `P4_GUEST_CHECK` run of the gates (it faults the first host pointer stored into guest memory) is not in the land check or the nightly. Three golden-replay functions are inconclusive on AArch64 (6.7).
6. **The reference.** The i386 build is what the golden traces, ovdiff and the gate limits were calibrated on. The 64-bit gates pass with the same limits, the digest is equal, and `ovdiff64` and the replay run on the 64-bit block (6.4); keeping `p4pc` as a nightly row (not a land-check shard) for one release cycle costs little and shows a divergence the day it appears.

Machinery the 64-bit build does not need (all of it is i386-only):

- **The fixed addresses.** Windows: the `.ps2ram` PE section (`src/platform/os/win32_ps2ram.S`, image base 0xF0000, 64 KiB sections, `/sectionlayout`, no ASLR; CMakeLists.txt around line 109) that makes the exe image own 0x00100000..0x02000000, and `p4_ps2_exe()` which 49 places in the tests and CMake call to link it. Linux: `mmap(MAP_FIXED_NOREPLACE)` of the PS2 RAM range (`os_posix.c`, the `address range in use` failure and its `vm.mmap_min_addr` assumption). The 64-bit guest block is one allocation at any address: Android and Apple, which forbid low fixed mappings, only build 64-bit.
- **The identity pointer model.** `P4_G2H`/`P4_H2G`/`P4_FN_*` are identity macros on 32-bit (`guest.h`, `compat.h`, 21 `P4_GUEST64` conditionals in `crt.c`, `image.c`, `gport.c`, `guest.c`, `walk_script.c`, `p4pc_viewer.c`, `android_main.c`): they become unconditional, and the `#if`s go.
- **The 32-bit toolchains.** `-m32` and the i386 multiarch libraries (SDL3, FFmpeg, Mesa, libc) in `ci/Dockerfile`, the i686 llvm-mingw build and Wine32 of `ci/win32` (`build_deps.sh`), the coroutine switch of `coro_asm.c` and the `ucontext` fallback, `--coff` in `gen_port.py` (leading underscores, stdcall `@N`, `R_386_*` and `IMAGE_REL_I386_*` relocations) and its i386 passes: the x87 result-pop retyping of `void` prototypes that return floats, the 16-byte struct return, the i386 cdecl varargs.

What dropping i386 simplifies: one way to compile the mirror (the rewriter; `P4_PORT64` stops being an option); the land check loses its `main` (i386 Linux build and ctest, about 330 s of critical path warm) and `win` (Win32 and Wine) shards and keeps `x64`, `x64w` and the a64 scope, and the CI images lose the 32-bit libraries; about 1000 s of cold rebuild of the 64-bit mirror stays, but nothing else rebuilds in parallel with it; every port native can assume that a guest address is not a pointer. What it costs: a host pointer no longer equals a guest address, so debugging with gdb or the retail address list needs the base (`p4_guest_base`, the `ram_diff --ram-ptrs` flag); the reference build goes; 32-bit Windows users (Windows 10 and 11 on a 32-bit CPU are not a realistic audience [inferred]) are dropped.

Plan (steps 1 and 2 are done above: x64 GPU gates and shards in the land check, i386 in the nightly, Linux package script; the Windows package default is PortRelease's): (1) smoke-test both packages on a clean machine; (2) ship one release with both Windows exes; (3) done: the 64-bit twins of the host tests (6.8); (4) if the user later drops i386: delete the i386 targets and the machinery above in one change per group (fixed addresses and `p4_ps2_exe`, the identity macros and their `#if`s, the toolchains and images, the gen_port i386 passes), each checked against the gates.

### 2.17 The rewriter's rule counts are a contract

`unit64.py build` writes `<object>.census` next to each object: the rewriter's per-rule counts for the unit (`abs-data:ptr=9 read-ptr=13 layout-asserts=5 ...`).
`config/port64_census.txt` (x86_64 Linux) and `config/port64w_census.txt` (x86_64 Windows) hold the reviewed counts of all 1231 units, and the build of the mirror
(`p4decomp64`, `p4decomp64w`, so every 64-bit executable) fails when a unit's counts differ: a rule that stops firing, fires elsewhere or a different number of
times means the decomp, a pin or the rewriter changed under it, and nothing is silently skipped (`ctest port64_census` runs the same check on the Linux objects). The
check prints the first differences (`unit: rule X: baseline a, now b`); after review, `python tools/port64/unit64.py report --dir <build>/gen/obj64/units
--census-refresh config/port64_census.txt` (`obj64w` and `port64w_census.txt` for Windows) rewrites the baseline and the diff is committed with the change that caused it.
The counts that come from the layout oracle (`layout-asserts`, `record-without-oracle`) are the compiler's own record dump, so they depend on the clang major (19 and 23
differ in `mdlManager.c` and `y_CmbCardEff.c` at the d19f9e22 pin): the baseline file is the full table of the CI compiler (`# clang-major: 23`, the llvm-mingw clang of the p4pc-ci image, which every land
and nightly build uses) followed by `patch` sections that list only the units that differ for another supported major (19, a distribution clang on a developer machine).
A refresh refuses to record a unit whose `record-without-oracle` count is higher than in the file it replaces (for that clang major): more records without
the layout oracle means the oracle compile of that unit failed, a defect to fix rather than a baseline (2026-11-04: a Bionic `stdio.h` conflict broke
`btlEffect.c`'s oracle and the Android refresh recorded 12 instead of 7, which every later check then accepted).
A compiler of a major with no section fails the build with the command that reviews and adds one (`--census-refresh <file> --clang <that clang>` writes the patch); nothing is
skipped silently. Only the units of the current build are compared (the build passes its unit list, `--active-units`): a `.census` left by a unit that is no longer compiled cannot stand in for it, and an active unit without one is an error. A missing selected baseline is a configure error, including Android; it must never silently disable the guard. `-DP4_CENSUS_CHECK=OFF` (a configure warning) skips it only for explicit experiments and still writes the active-unit list for reviewing a new baseline. AArch64 has its own (`config/port64a64_census.txt`, from the p4pc-ci-a64 image's clang 19); Android AArch64 uses `config/port64android_aarch64_census.txt`, reviewed with the NDK r27 clang 18. Android selection follows `ANDROID_ABI`, not the desktop architecture cache.

The Android clang18 `src/sce/rofs_mai.c` row includes the native ROFS error
formatter installed by `ba899d5b`: its two `sprintf` branches require two ABI
call adaptations and two printf masks, with thirteen macro/foreign skips and
zero unsupported constructs. This is an explicitly reviewed profile refresh,
not a decomp-source change: `rofs_mai.c` is unchanged between the d19f9e22 and
854e43d2 pins. Linux/Windows x86-64 already recorded the two formatter calls;
their macro counts are profile-specific and must not be copied into Android.
The existing iOS renderer-syntax check is not an iOS mirror/census qualification.

Linux AArch64's clang19 row now records those same two ABI adaptations and two
printf masks, with 43 macro/foreign skips. The Linux x86-64, Windows x64 and
Android rows were reviewed too and retain their existing target-specific counts
(43, 20 and 13 macro/foreign skips respectively); no cross-target blanket refresh
is valid. Only the stale Linux AArch64 ROFS row changes.

All four reviewed profiles block every land. The desktop Linux/Windows builds
check their full mirrors. Parallel `a64-generation` and `mobile-generation`
shards use the real Linux AArch64 and NDK toolchains to build
`p4decomp64_generate`, including the complete active-unit census check, without
compiling mirror objects or running qemu game tests. CI explicitly re-enables
`P4_CENSUS_CHECK` in warm configurations. Each generation shard has its own
900-second build budget and existing lock-wait allowance; native/game budgets
are unchanged. No iOS or macOS census is claimed without its actual SDK/oracle.

The sole PC `func_004c85a0` takes an unused scalar mode, not the deleted
device-model dispatcher's directory pointer (which the default branch always
discarded). Its three callers therefore lose one pointer-argument adapter:
`sdkCdvd.c` has argument/call counts5/4, `h_cdvd.c`6/5, and `sdkPak2.c`10/8.
The saved Linux census confirms exactly those deltas with no unhandled
constructs. All four64-bit profiles carry this shared signature change while
retaining their own layout, macro/foreign and other rule counts.


The walk-script planner's collision probes (`src/platform/nav_probe.c`: the game's own sphere and ray tests) now exist on the 64-bit builds too. They were i386 only
and the walker fell back to the cell grid there, which cannot route through a castle floor: `castle_3f_0418` ended in a battle field on x86_64 while the i386 build
reached the floor. With them 28 of the 29 story checkpoint gates of the story lane's series reach their target on x86_64.

## 3. Prototype: the kernel task list

Subsystem: `decomp/src/Kernel/sdkTask.c`, 1060 lines, 23 functions: the task struct (`KwlnTask`, 84 bytes, 9 pointer and function-pointer fields), three doubly linked lists whose heads are pointers at retail addresses (`iGpffffba04` and friends), parent/child/sibling links, update and destroy callbacks, an allocator reached through function words in retail data (`D_008873F8[0]`, `jtbl_008873FC[0]`), pointer sentinels (`(KwlnTask *)-1`, `(SdkTaskUpdate)-1`) and a callback whose return value is stored as the next callback.

**Scenario** (`tests/port64_task_test.c`): initialise the pool, create eight tasks (three trees, one with children created through `kwlnTaskCreateChild`), run ten frames (callbacks return 0, -1 or the address of the next update function, some tasks delayed), look up by name, destroy by name and by hierarchy, look up a destroyed name. Every frame it writes every field of every task on all three lists, as guest addresses and callback names, and each assert, allocation and free; the scenario is 227 lines of trace.

**Three executions, one trace** (ctest `port64_task`, `port64_task_retail`):

1. **i386**: the mirror unit as the shipping build compiles it, guest address == host pointer.
2. **x86_64**: the same unit rewritten by `gptr_rewrite.py` (`tools/port64/build64.py` rewrites, compiles with `P4_GUEST_CHECK`, links), guest RAM an allocated block. 292 rewrites in the unit: 89 pointer loads (11 of function pointers), 40 pointer stores (2 of function pointers), 13 NULL stores, 10 slot-to-slot copies, 20 declarations retyped (9 fields, 11 variables), 116 retail-data references, a `(u8 **)` cast and 3 integer/pointer conversions; zero pointer/integer warnings left; 18 layout assertions pass.
3. **Retail**: the same scenario with the unit's functions replaced by thunks into the R5900 interpreter executing the retail instructions of `kwlnTask*` from the loaded `SLUS_217.82` over the x86_64 block (callbacks and the allocator re-enter the host through the interpreter's call hook; libc and the printf are hooked).

All three print the identical 227-line trace (hash `53694291`), with retail data loaded. That is retail = i386 native = x86_64 rewritten native on a module with lists, parent/child links, callbacks in guest memory, sentinels and function words in retail data. `P4_GUEST_CHECK` was on for the 64-bit runs: no host pointer reached guest memory. The rewrite found no case the retail run would have caught: the retail run exists to catch one.

Not shown by the prototype's run (the whole mirror compiles, but only this unit executes): stacks in guest memory (the unit stores no local's address), static initializers, macro-heavy code (RenderWare), the cross-unit ABI adaptation at run time, threads, and the platform layer on arm64. They are M2 onward.

## 4. Staging plan

Each milestone leaves the i386 build and every existing gate unchanged and is verified by something that already exists or is added in the milestone.

| M | Deliverable | Verified by | Size [estimate] |
|---|---|---|---|
| **M0** | Model, `guest.h`/`guest.c`, rewriter, whole-mirror rewrite and census, i386/x86_64/retail prototype, CONTRIBUTING rule | `port64_task`, `port64_task_retail`, `tools/port64/*.py` runs quoted above | done |
| **M1** | The rewrite is a build step: CMake target `p4decomp64` (per unit `abi`, then `build`, with depfiles; on by default, `-DP4_PORT64=OFF` to skip) compiles all 1231 units for x86_64 beside the i386 build; macro layer replaced by rewriting preprocessed text; init functions, `p++`, pointer-to-array types, word types, ABI adaptation, layout asserts | All 1231 units compile for x86_64 with zero pointer/integer diagnostics and every layout assertion passes (`p4decomp64`); ctest `port64_mirror` fails on any unsupported construct not in `config/port64_unsupported.txt`; CI builds it on every land | done |
| **M2** | Runtime: guest block at boot, loader without patching, function table from `p4pc_native_table`, port-owned data and string pool in the port half, init functions, guest stacks and a CPU-specific context switch (x86_64 first), `kernel_threads_test` on it | Cross-target [startup and kernel contracts](platform/threads_kernel.md) on x86_64 Linux; `kernel_threads_test`, `memcard_test`, `files_test` built for x86_64 | 3 weeks |
| **M3** | Port code audit by seam: the 1016 pointer-taking port natives and the 51 mirrored port structs, in order of the layout assertions that fail (`P4RwDevice`, `P4RwRaster`, `P4SjUni`, then audio, files, movie, GL); seams switch to `u32`; callbacks registered through the table | Each lane's existing tests built for x86_64; `P4_GUEST_CHECK` clean on the boot and the gates; the callback check in the table generator | 4 to 6 weeks, in parallel with M2 after its first week |
| **M4** | `ovdiff`, `replay.c` and `sigs.txt` thunks on the guest block | `ovdiff_subset` on x86_64 with the same verdicts as i386 (464 functions); golden replay `-R` counts equal | 2 weeks |
| **M5** | Linux x86_64 playable | `newgame_to_field`, battle, camp, save gates and the scene MADs within the i386 numbers (title 1.14, field 1.78 ... docs/porting.md) | 2 weeks of triage [estimate: dominated by what M3 finds] |
| **M6** | Windows x64 (llvm-mingw): coroutines on guest stacks, no `.ps2ram` | the same gates under Wine, then on the RTX 4080 machine | 1 week |
| **M7** | arm64: macOS, Android, then iOS: FPCR float mode, context switch for arm64, unaligned/128-bit review, SDL3 per platform; the renderer API per platform is the renderer lane's | Scene MADs on an arm64 Linux host first (cheapest), then each OS | 3 weeks plus the renderer lane |

### Cost estimate

Grounded in 1.3 and 2.3, not in a guess of effort per function: the 86,000 rewrites that matter are mechanical and already done by the tool; what is left is the part the tool flags or cannot see.

- **M1** (done) was the rewriter's unsupported constructs, the macro problem (solved by preprocessing, not by a macro catalogue), the ABI pass and the layout-assertion triages. The build step costs about 20 CPU-minutes for the whole mirror at `-O0` (2.5 minutes on 24 cores) because it parses 1231 units twice (`abi`, `build`).
- **M2 and M3** are where the unknowns are. 1016 port natives take pointers, but a seam only changes if it reads a pointer out of what it is handed (pointer fields) or keeps the pointer: the 15 failing port files and the 51 mirrored structs bound that list; the rest only pass pointers through `P4_G2H`. Each lane owns its seams; the audit itself is mechanical (`inventory.py` already prints it).
- **Verification** is cheap because it exists: the retail interpreter runs over the block unchanged (three lines), and the i386 build is the reference for every gate.
- **Risk-weighted total [estimate]:** 4 to 5 engineer-months to Linux x86_64 playable (M1 to M5), plus 1 week for Windows x64 and 3 weeks plus the renderer for arm64. The risks that could move it are guest stacks on Windows and arm64 (M2/M7), and the number of host pointers stored in guest memory that `P4_GUEST_CHECK` has not yet met outside the task unit (M3).

## 5. Open decisions and risks

1. **Scratchpad and device windows** are windows backed by host memory, not by the block (2.1): reads and writes work, but a pointer into them is not a block address, so code that does arithmetic across a window boundary or hands a window address to the interpreter's page tracking needs a look in M4.
2. **Rewrite soundness is compile-time only.** A conversion the rewrite misses where both sides are integers (an address passed through an `s32` that a callee later uses as a pointer through a cast the AST shows as an int-to-pointer cast) is caught; one hidden in an inline-asm operand or a hand-built table is not. Runtime coverage is the gates and `P4_GUEST_CHECK`.
3. **Guest stacks need our own context switch** on every platform; the prototype did not exercise it. Fibers and `ucontext` are out.
4. **Callback ABI** is checked by the table generator only for functions that are stored; indirect calls through hand-built tables are covered by the replay and the gates.
5. **`P4_H2G` cost.** Both conversions are an add and a select (NULL); the optimiser removes most pairs. If profiles show it, the rewriter can hoist the translation of a loop-invariant base.
6. The prototype's retail run uses ovdiff's R5900 interpreter and so shares its blind spots (it models what the game uses, `docs/override_diff.md`); the gates remain the real check.

## 6. M3: port-owned code under the guest-pointer model

Owner: lane Port64Seams (`src/` except the boot files Port64 keeps for M2: `image.c`, `loader.c`, `crt.c`, `platform/os/`, `kernel_init.c`, `ps2_kernel.c`, and the context switch in `kernel_thread.c`; the overrides are spliced into mirror units and rewritten by `gptr_rewrite.py`, so they are Port64's).

### 6.1 What counts as a seam and the rules that were applied

`tools/mipsdiff/sigs.txt` has 1311 override and port-native functions; 1016 take or return a pointer. 477 of those are overrides (rewritten with the mirror), **539 are port natives defined under `src/`**: those are the seams of this section.

- **Natives keep the classes of their definition.** A `T *` parameter is a host pointer in the register ABI; an `int`/`u32` one is a guest address. `sigs.txt` is generated from the definitions and is the authority; the rewriter's ABI pass adapts the mirror's call sites to it (an int or K&R argument passed to a `p` parameter gets `P4_G2H`, a `p` result used as an integer gets `P4_H2G`). The decomp declares many of them m2c style (`tools/port64/seam_abi.py` without arguments lists the declarations that disagree with a definition: informational, the rewriter's ABI pass handles them); the gates of 6.3 check the other direction, port code against the callee's real classes.
- **What a native reads out of guest memory is converted.** A pointer field of a guest object is a 4-byte guest address: `P4_G2H(word)` when read, `P4_H2G(ptr)` when stored; a function word is a guest code address (`P4_FN_G2H` / `P4_FN_H2G`); retail data is reached by address (`P4_GDATA`, `P4_GARRAY`, `P4_GPDATA` in `include/p4pc/gmem.h`), not through a linker symbol, which exists on i386 only.
- **Objects the game keeps a pointer to live in guest memory.** A host `static` or `calloc` object whose address is stored into a game struct cannot be represented: `p4_gport_alloc` (port-owned guest storage, `src/platform/gport.c`: the top 16 MB of the block on a 64-bit host, `calloc` on i386) holds the SJUNI objects, the IOP RAM the stream-joint chunks point into, the AXRNA objects, the movie players, the frame rasters, the work area of the ADXF partition load, the RenderWare memory table; `p4pc_heap_*` (the game heap, guest memory) holds what is freed (raster pixel shadows and palettes, ADXF handles, the movie frame buffer, the SFMPVP decoder state). Host-only objects (mixer voices, libav contexts, FILE handles, GL names, the raster slot table) stay in host memory and are never converted.
- **Mirrored structs have the i386 layout and an assert for each field offset**: `P4SjUni`, `P4SjCk`, `P4SjCkcn`, `Ck`, `P4RwDevice`, `P4RwRaster`, `P4RwTexture`, `P4RwImage`, `P4RwRasterDims`, `P4SvmSvrFunc`, `kt_thread_param`, `AdxmThreadParam`, `Crprm`, `P4SfCk`, `ViBuf`; the pointer members are `u32`.

### 6.2 Seam table

"Natives" counts the non-static functions a file defines; "ptr" those with a pointer in the signature. converted = every pointer read from, stored into or handed out of guest memory by the area goes through the API above and the area's guest-visible objects are in guest memory; verified = what ran.

| Area | Files | Natives (ptr) | Converted | Verified |
|---|---|---|---|---|
| `src/cri` (ADX, AHX, AIX, SJ, SFD, CVFS, SVM, PL2) | 24 | 508 (397) | all: `cri_mem.h` (`M8/M16/M32` by guest address, stream-joint vtable called through `P4_FN_G2H`, `Ck.data` is a guest address), SJUNI in guest storage with its vtable at the retail address, AHX/AIX/ACS/ADXM/ADXT/PL2/SFD object fields, strtol end pointers as guest slots, SVM callback table words | x86_64 `src_check` 0 diagnostics; x86_64 run of `cri_acs`, `cri_acssnd`, `cri_acs_step`, `cri_adxm`, `cri_init`, `cri_adxt_start`, `cri_stm`, `cri_cvfs` (the standalone ones, `tools/port64/test64.sh`): pass; i386 ctest (`cri_*`, `adx_wav`, `ahx_wav`, `aixp_wav`, `axrna`): pass. `axrna` and `cri_adxi_rpc` are converted to guest storage and wait for the 64-bit mirror link to run on x86_64 |
| `src/platform/files` (VFS, ADXF, ROFS, cdvd) | 9 | 105 (71) | ADXF partition data words at their retail addresses, handles and the work area in guest memory, ROFS interface table by address | x86_64 `adxf_partition`, `files` (skip: no disc) built and run; i386 ctest `adxf_partition`, `files_disc`, `sfd_decode`: pass |
| `src/platform/memcard` | 1 | 13 (11) | none needed: host pointers to guest buffers only | x86_64 `memcard` pass; i386 `memcard_roundtrip` pass |
| `src/platform/pad` | 2 | 19 (11) | none needed | x86_64 `pad_keyboard` pass; i386 pass |
| `src/platform/sound` (sndcom, sdr, mixer, AXRNA) | 9 | 87 (53) | IOP RAM and the AXRNA objects in guest storage, joints through the vtable helpers, SifSetDma source as a guest address | x86_64 `sdr`, `mixer_device_dummy` pass; i386 `sdr`, `mixer_device_*`, `sebank`, `movie_audio`, `axrna` pass |
| `src/movie` | 7 | 69 (67) | players and the SFMPVP state in guest memory, frame buffer in the game heap, `Crprm` layout, error message word through `p4_gport_str` | x86_64 `sfmpvp`, `mwply`, `sfd_decode` built (skip: no disc); `movie_smoke` and `movie_task_smoke` converted to guest storage (they link the mirror); i386 `sfmpvp`, `mwply`, `movie_start`, `movie_task`: pass |
| `src/render` (p4gl) | 16 | 108 (61) | `P4RwDevice`/`P4RwRaster`/`P4RwTexture`/`P4RwImage` to the guest layout, all function slots guest code addresses, raster pixels and palette in the game heap, frame rasters in guest storage, camera/atomic/ptank pointer fields read as guest addresses, device handler out-parameters written as guest addresses | x86_64 `src_check` 0 diagnostics, all layout asserts pass; `render_smoke` converted (synthetic atomics, PTank records, Im3D globals, rasters in guest storage; render-state integers as `P4_G2H(n)`); i386 `render_smoke`, scene gates: pass |
| `src/viewer` | 2 | 2 (2) | pointer data words, camera/frame fields, sequence parameter block in guest storage | `src_check` x86_64 and i386 |
| `src/platform` (kernel model, heap, libc, RW natives, harnesses) | 33 | 215 (102) | heap debug header sized for the host pointer, memory table in guest storage, `strtol`, RW natives (`rx_heap_realloc`, image stream, rtanim, skin, device/frame register, plugin constructors), harnesses and `newgame_gate` through retail-data macros, thread param layout | x86_64 `kernel_threads`, `vu0_helpers`, `fpbit`, `vag` pass; i386 pass |
| `src` root | 4 | 9 (5) | Port64's (M2): loader, `crt.c`, `image.c`, `vu0.c` is host math | `src_check` |

`tools/port64/src_check.sh` (x86_64 and, with `P64_TARGET`, i386) reports 0 diagnostics with `-Wpointer-to-int-cast -Wint-to-pointer-cast -Wint-conversion -Wpointer-sign -Wincompatible-pointer-types -Wshorten-64-to-32 -Wint-to-void-pointer-cast -Wvoid-pointer-to-int-cast` over every `src/` file except `os_win32.c` (needs the Windows headers). `tools/port64/src_check.sh` also takes the test sources (`tools/port64/src_check.sh out tests/*.c`): the tests of the overrides (`rxheap_test`, `rtanim_test`, `promoted_leaf_test`) are Port64's and still report int/pointer casts. The compiler cannot see a pointer read through a `memcpy` of a pointer-sized object, `*(T **)` field idioms, retail addresses cast from integer literals, or storage that a game struct points at; those were found by the audits this section's patterns came from (`grep` for `(T **)` casts, `memcpy(&ptr`, `(T *)0x...`, `__asm__("D_...")`, `extern` retail data, `sizeof(void *)`, `uintptr_t`) and checked by reading each site.

### 6.3 The gates, and what is left for other lanes

Port code is not rewritten, so every call that crosses between port code and the mirror has to agree with the callee's register classes by hand. Three gates keep that true (ctest, empty baselines):

- `seam_abi --check`: a prototype of a port native anywhere in the port (headers, sources, tests) agrees with the definition on pointer versus integer for every parameter and the result; a `void *`/integer parameter of a port native that its body casts to a function pointer is reported (declare it as a function-pointer type, or the call sites convert it as data).
- `seam_abi --check --mirror abi.json` (`seam_abi_mirror`, the build's merged table, decomp aliases included): a port prototype of a decomp function agrees with the mirror's classes. An m2c `Uint32` parameter takes a guest address: pass `P4_H2G(p)` and declare `uint32_t`; a host pointer would arrive truncated. It found, among others, the ACSSND leaf calls, `func_004bd6a0` messages, `ACSSND_Create`'s pointer result, `kwlnTask*`, `datUnitCreate*`, `RwStream*` results, the sj error forwarder, and the packets handed to `sequenceRequest`.
- Indirect calls through a guest code address use `p4_fn_g2h_sig(word, P4_SIG(n, r, a0..))` with the classes the call site passes (51 sites in `src/`), and a function the port stores into guest memory that has no retail address is registered with `p4_fn_h2g_sig(f, "sig")` (sj error function, device unassigned slot, ADXM thread entry, fade hooks, AHX detach).

Settled: integer payloads. `RwRenderStateSet`/`RpSkyRenderStateSet` take the value as a `u32` that is never translated (a raster is its guest address); thread entry arguments are integer words (`StartThread(id, uint32_t arg)`), the entry is called with the integer class and the table adapts to a pointer-typed entry; callbacks registered through typed registrars (`SVM_SetCbErr`, ADXM lock callbacks, `RwFrameRegisterPlugin`) are declared as function-pointer types.

Open:
- **Device windows.** `kernel_init.c` (EE TIMER0 registers, 0x10000000) and `ps2_dma.c` (`sceDmaGetChan` channel registers, 0x1000xxxx) reach the EE register page through `P4_G2H`; the block must back that page (docs section 5.1).
- **Mirror functions known only under a decomp name** (`ACSSND_GetStat` for `func_004bee38`): the mirror gate resolves them through the `// FUN_<addr>` markers; a function with no parameters in the table (`()` K&R) has nothing to compare.
- **Float callbacks.** A float or double parameter or result is class `x` in a signature (`P4_SIGX(i)`, `P4_SIGXR`): the adapters take the six integer and eight xmm registers, count the integer registers over the non-float parameters and return {rax, xmm0}; stack arguments (more than six integers or eight floats) are not handled (tests/fn_adapt_test.c).

### 6.4 M4: ovdiff and the golden replay on the 64-bit block

`tests/CMakeLists.txt` builds `ovdiff64` (`-DP4_PORT64_LINK=ON`: `tools/mipsdiff/ovdiff.c` and `r5900.c` linked with the rewritten mirror, the x86_64 port
objects and the 64-bit tables) and runs `ovdiff_subset64`. What changed against i386, all identity on i386:

- A native is called through the System V registers (`ovd_sysv_raw`, on a private stack that is guest memory, so the address of a local a native passes on is a
  valid guest address): the classes come from the tables' `sig` (the compiled function's real classes; sigs.txt only where it has a float, double or 64-bit class),
  a pointer-class slot is translated from its guest address (a retail code address a native stands for becomes the host function), and a pointer result goes
  back to a guest address, so the harness compares guest values on both sides.
- The arena, the camera, the engine allocator slots, the dirty-page tracking (`mprotect` on the block) and the replay's recorded writes go through `P4_G2H`;
  the harness callbacks (`ov_malloc` ...) are function words registered with signatures; the replay's libc heap sits at `0x03C00000` inside the block.
- `p4_guest_check_off`: the cases write random words into pointer slots; they round-trip through `P4_H2G` unchecked, as on i386.
- Trap stubs log a host pointer argument as its guest address (`gen_port.py tables --ptr64`).

Status (`ovdiff -n 20` over `tests/ovdiff_subset.txt`, 472 functions): 469 are ok (any ok class) on x86_64 as on i386, and the same two are not: `func_003d0460` (no valid case on either) and `func_0039f050` (a trap-stub arity artefact,
listed in `tools/mipsdiff/expected.txt`: the native passes the one argument it has to a stub that logs four, and on x86_64 the registers past the first hold leftovers where
i386 has zeroed stack words). What it took, beyond the harness above:

- the 64-bit mirror goes through the EE float rewrite (`tools/ee_float_cc.py` treated only `*.c` sources; the units are compiled from `*.i`), originally qualified at `-DP4_PORT64_OPT=O2`:
  closes `func_003dc490`, `func_003dc510`, `func_003df9f0`, `func_003f0510`. The current O0 compile-only check uses the launcher too when `P4_EE_FLOAT` is enabled;
- the float class in signatures (`P4_SIGX`, `P4_SIGXR`, letter `x`): closes `func_003d59d0`, `func_003d5e90`;
- `ptr + u32`, `ptr - u32`, `ptr[u32]` and `ptr += u32` read the word as signed (gptr_rewrite `signed_offset`): a negative offset held in an unsigned word wraps mod 2^32 on the
  PS2 and reached 2^32 + offset on the host (`mdlEffectUpdateParticleTrail`, `func_003b4c10`, and the wrapped stores of `func_00416bc0`);
- the harness: as on i386 nothing is mapped below the image or above RAM (a wild length faults on both sides), integer slots are zero-extended (a `size_t` parameter sees
  what a native's own caller passes), a pointer slot below RAM is passed raw (a native computes NULL + offset: the same number as a host address), and a native that
  dereferences a number below 4 GB outside RAM built its pointer on NULL (`func_003bcbe0`: a0 = 0, a1 = a guest address as the offset): an invalid case, the model has
  no such address.

Golden replay (`tools/remote_replay.sh` on i386, the same replay of `ovdiff64` on the disc host; golden files stay there): 318 ok + 16 ok (float tolerance) on x86_64,
318 + 15 on i386; 7 MISMATCH against 8 (the same set, minus `func_004c7260` which x86_64 matches); of the other rows that differ, the x86_64 side is better (`func_003c95a0`,
`func_004be4a8`, `ACSSVR_CreatSnd`: fewer crashes or stubs) or documented: `mdlEffectInitParticleState` (returns a 128-bit value: in xmm0 on x86_64, the harness reads rax),
`func_00446ed8` (the recorded va_list of a retail caller), `func_004c6eb0`/`func_004c6ee0` (the return is the address of retail's device record; inconclusive).

#### 6.4.1 Reviewed catalog refresh for decomp `363eab8a`

The `854e43d2` → `363eab8a` cutover (`a233ac05`) updated several names but
did not fully regenerate `tools/mipsdiff/sigs.txt`. The reviewed full catalog
contains **6,150 rows: 1,400 native/override `o` and 4,750 callee-only `d`**,
versus 6,252 before, with no duplicate addresses. Its SHA-256 is
`79b91bf24c0ca08a93b1ccc304332561ba864f0d7d70bd4d2f04b8d6341af787`.
This changes catalog data, not source implementations, refs or test limits.

Of the **218 changed addresses**, **176** are grounded in exact upstream
declaration or symbol-table lines between those pins:

| Exclusive category | Rows | Meaning |
|---|---:|---|
| Now-default callee signatures | 103 | Corrected integer/pointer results and general-register parameters no longer need a special `d` row; the runtime functions are not removed |
| Arity corrections | 55 | Actual callback/SDK inputs replace incomplete prototypes; includes the corrected `ADXB_SetAhxInSj` / `ADXB_AhxTermSupply` bindings |
| Same-arity class/order corrections | 10 | Pointer/scalar widths, signed classes and argument ordering follow the recovered declarations |
| Return-class corrections | 4 | `datCalcGetBadStatusNoDownById`: integer; bank/list creation: pointers; shuffle normalization: float |
| Other upstream rename | 1 | `004c3c00` is `ADXB_GetFmtBps` |
| Private-to-canonical providers | 2 | `criSetVoiceCounts` and `RFMAI_CnvU32plusU32toS64` use their canonical decomp rows after redundant private providers were removed |
| Newly resolved native CRI provider | 1 | `ADXT_GetNumChan` is emitted from its actual native definition |

The relevant upstream evidence includes `2bbc2a90b` (interfaces/symbol roles),
`7d64e64ff` (pointer/float results), `d94be54b4` (AHX/formatter contracts),
`deabd5be6` (RFMAI packing), `4ef90cefb` (AI word widths), `ef9ec11ba`
(packet callback results), `614dbf3f0` (AI search results), and `640e9e450`
(plugin/stream callbacks), plus the port-side cleanup in `a233ac05`.

The remaining **42 rows are older native-catalog drift**, not attributed to
this pin bump. PortSettingsMenu reviewed and approved them separately: 18
existing native additions; 18 dead `func_` provider rows removed; two integer
returns and one scalar-mode parameter corrected; two old ADXF private rows
replaced by their existing named `d` providers; and `0044e830` restored to the
existing `uint32_t(uint64_t)` native provider (`i q o`). Soft-double bits retain
integer-register `q` transport, unresolved varargs remain `?`, and declared
scalar ADXF handles do not acquire invented pointer classes. No compatibility
aliases or census/reference refreshes are part of this synchronization.

Generation uses the exact pinned mirror/ABI and CI compiler:
`sigs.py --gen <gen> --abi <gen>/obj64/abi.json --clang /opt/llvm-mingw/bin/clang
--out <fresh>/sigs.txt --record-report <fresh>/aggregate.json`.
The frozen base image is
`sha256:1c74a3c999b3c4003f4e8a1aaa5077d4ece619b2a5d76c77dfa31632f5b310c9`.
Historical `sigs_audit.json` proofs remain unchanged. Subsequent decomp
cutovers must regenerate and compare the complete catalog, not just rename a
few rows. The staged orchestrator cutover adds a per-profile derived-artifact
guard for that qualification requirement.

### 6.5 The mirror-linked tests on x86_64

`render_smoke64`, `movie_smoke64`, `movie_task64`, `movie_start64`, `axrna64`, `cri_adxi_rpc64`, `adx_wav64`, `ahx_wav64` and `aixp_wav64` (tests/CMakeLists.txt,
`p4_test64`, built with `-DP4_PORT64_LINK=ON`) link the rewritten mirror and run against the retail disc (`P4PC_ISO`, `P4PC_ELF`). All nine pass on x86_64
(Linux, Mesa llvmpipe ES 3.2): the wav tests reproduce the references with 0 LSB deviation, `movie_smoke64` compares 121 frames byte for byte with the CPU
conversion, `render_smoke64` and `movie_task64` render. Their `main` runs on a stack from `p4_guest_stack_alloc` (`p4t_on_guest_stack`, tests/guest_test.h, the way
src/crt.c runs the game): a local handed to the mirror as an m2c `int` parameter must be a guest address, and the process stack is not in the block.
They are gates: the land check builds the mirror64 objects at -O2 (`tools/p4pc_ci_run.sh`) and runs every `port64` test (these nine, `fn_adapt64`, `ovdiff_subset64`, `boot64`).

### 6.6 The viewer on x86_64

`p4pc_viewer64` (the renderer's reference tool) runs the game's title and field scenes on the 64-bit mirror: `title_mad64` measures 1.1364 % against the PCSX2 frame
(limit 1.25) and `field_mad64` 1.7749 % (i386: 1.7747 %, limit 1.9); both are gates (label `port64`). What the port needed beyond the tests' changes: its `main` runs
the engine on a guest stack (`viewer_main` on a `p4_guest_stack_alloc` coroutine, as `src/crt.c` does for the game); records and buffers the game keeps (the title task's
work and task records, a pack's RMD copy, the RwMemory of a world stream) are game-heap memory with guest addresses, and a pointer stored in a retail record goes through
`P4_H2G`; the camera functions the mirror defines as `int` (`kwlnGetCamera`) are declared `uint32_t` and translated (`seam_abi --mirror` now compares the result
class of a prototype with no parameters too, which is how this and the same bug in `battle_harness.c` surfaced).

The rewriter rule that the field scene needed: an m2c sum of two guest addresses kept in a pointer (`p = (u8 *)(index * 4); p += (s32)base;`, `FldResource_StepPartLoad`)
is formed on guest addresses and translated once (`gptr_rewrite.py guest_sum`: `p4_g2h(p4_h2g(p) + (u32)(int))`); `NULL + host address` is not that sum, and index 0 made
the pack name `f000_-15999.rws`.

### 6.7 More 64-bit gates, and AArch64 under qemu-user

**x86_64 gates added.** `battle_menu64`, `camp_menu64`, `save_load64` (p4pc64) and the five `scene_*64` field gates (p4pc_viewer64, `p4_scene_gate` in tests/CMakeLists.txt
registers each scene on both viewers) join `title_mad64`/`field_mad64`; with `newgame_to_field64` they run in the land check's main scope. Four things they needed:
a `void *` read from guest storage and cast to a function-pointer type is a function word (`p4_fn_g2h_sig`: the sector render callback was called with a guest address where it
takes a host pointer); every function the ABI table knows is registered with its signature at start (`p4pc_hostfn_table`: a RenderWare routine the decomp names has no retail
address, so a stored word for it carried no signature and was never adapted); a call to a definition with a struct parameter by value (`datScreenWipeSetOffset`) adapts its
other parameters (`abi-partial-row`); and `tools/mips2c.py`'s translation of retail libm kept a pointer spilled to the frame as a 32-bit word (`sw a1, 320(sp)`): on a 64-bit
host the frame word keeps the low half and a shadow slot the register (`shw`, regenerated `src/platform/ee_libm.c`), which is what made four scene gates crash only when the
stack's address had its top bit set. The 128-bit-return and float-adapter notes of 6.4 stand.

**AArch64.** `-DP4_PORT64_ARCH=aarch64` (CMakeLists.txt) cross-compiles the same tree with clang for `aarch64-linux-gnu`: the arm64 Debian multiarch libraries and
`gcc-aarch64-linux-gnu` are the sysroot, `-fsigned-char` (char is unsigned on AArch64, MIPS's is signed), `-ffp-contract=off`, `-mno-outline-atomics` for the port objects (the
libgcc helper the outlined atomics call is not a host function the tables know), the i386 `-m32`/SSE flags dropped, `llvm-objdump`/`llvm-nm` for the tables
(`gen_port.py --arm64`: AArch64 relocations, `b target` thunks, `%function`). tools/port64/a64.sh configures, builds, and runs the standalone tests and ovdiff under
qemu-aarch64; ci/Dockerfile.a64 adds qemu-user and the arm64 libraries (SDL3, FFmpeg, Mesa's llvmpipe) to the CI image, whose binfmt runs an AArch64 binary directly.
What AArch64 needed in the sources:

- `include/p4pc/eefloat.h`: the EE's nearest-rounded division, square root and reciprocal square root were x87 `long double` computations; they exist now as integer
  functions (`p4pc_ee_*_bits_int`, correctly rounded, ties to even, the denormal-grid edge included), used where there is no x87. tests/ee_float_test.c compares them with
  the long double formulas and, on x86, with the x87 path: 2.4 million results on x86_64 equal, 6 million on AArch64 against a software-quad reference.
- `include/p4pc/hostmath.h`: the EE mode in FPCR (RMode round toward zero, FZ); the libm host calls restore round-to-nearest around the call.
- `tools/port64/gptr_rewrite.py`: a `va_list` is a struct there; the decomp hands `args` on as a pointer (`(s32 *)args`, a `void *` parameter) in six places: the address of the
  object, which is what the array decay is on x86_64 (clang drops the cast from its AST, so one of them is fixed in the preprocessed text).
- `tools/mipsdiff/ovdiff.c`: the AAPCS64 call thunk, FPCR for MXCSR, the fault PC; the replay and interpreter need nothing (the interpreter's `long double` is a software quad). Its `-Q` soft-double probes use the same FP-control abstraction and AArch64 `fadd`/`fsub`/`fmul`/`fdiv`/`fcvt`, not unguarded x86 builtins or SSE asm; adding that diagnostic must not break the AArch64 nightly build.

Under qemu-aarch64 on an x86 host (the disc host for everything that reads the disc), measured:

| | AArch64 | x86_64 |
|---|---|---|
| standalone host tests (test64.sh: vu0_helpers, memcard, fpbit, vag, kernel_threads, ee_float, fn_adapt, adxf_partition, sdr, mixer, pad_keyboard, the nine cri tests) | all pass | all pass |
| the nine mirror-linked tests (render_smoke, movie_smoke, movie_task, movie_start, axrna, cri_adxi_rpc, adx/ahx/aixp wav) | all pass, wav tests 0 LSB | all pass |
| `ovdiff_subset` (472 functions) | 470 ok, `func_003d0460` as on i386 | 469 ok |
| golden replay: ok + float tolerance, MISMATCH | 319 + 14, 4 | 318 + 16, 7 (i386 318 + 15, 8) |
| `title_mad` / `field_mad` / sky scene | 1.1365 % / 1.4861 % / 2.6708 % | 1.1364 % / 1.4893 % / see 6.6 |
| battle_menu, camp_menu, save_load | PASS | PASS |
| boot with the disc | the opening movie | the opening movie |

The frames of the two architectures differ by at most 4 of 255 on 8.6 % of the pixels (mean 0.019 %): llvmpipe's NEON and SSE code. `movie_smoke` reads a brightness of
92.7 against 92.1 on AArch64: the converted frames equal the CPU conversion byte for byte, so the difference is libswscale's C path there (no accelerated yuv420p to rgba
conversion for AArch64 was found) against its SSE one. Three replay functions (`func_003999a0`, `func_00390520`, `func_003fe630`, plugin attach through
`_rwPluginRegistryAddPlugin`) are "no valid calls (1 crashed)" on both 64-bit builds (the i386 run calls them to an inconclusive row): the recorded RAM at their call has the
engine's free list `toolkitRegEntries` still zeroed (it is initialised by a call the replay does not make), so `_rwFreeListAllocReal` aligns with a mask of 0 and returns
`(u8 *)0 + 0`, which an arithmetic pointer cast turns into the block's base (`p4_g2h_off`) and not NULL as on i386; the caller's `if (entry)` passes and the write faults on the
protected first page. Same on x86_64 and AArch64 (ovdiff `P4_OVDIFF_NOFORK=1` runs the function in one process for a debugger: `qemu-aarch64 -g` and gdb-multiarch).

**Real arm64 hardware** (a Galaxy S20+, Snapdragon 865, Android 13, kernel 4.19, glibc binaries run through `ld-linux-aarch64.so.1 --library-path lib` from `/data/local/tmp`; no app, no GPU):
the standalone tests (every test of `test64.sh`, with and without the disc), the nine mirror-linked tests that need no GL (the three wav tests, `cri_adxi_rpc`, `axrna`; 0 LSB
against the reference), `ovdiff_subset` and the golden replay all pass, and their rows are identical to qemu-aarch64's, row for row (472 and 1320 rows, 0 differ): the integer
EE float helpers (6,000,000 results), the FPCR EE mode (round toward zero, flush to zero) and the `ovdiff` AAPCS64 thunk behave as the emulator says. `ovdiff_subset` takes 60 s there
(172 s under qemu on a loaded x86 host). The game itself (p4pc64, the viewer) needs GL and was not run on the phone.

What needs more than that: the game's renderer and audio on a real GPU and sound device, Apple (`__PAGEZERO`, Mach-O coroutine spelling, `pthread_jit`) and Android
(bionic, not glibc) specifics, NEON paths of the libraries, thermal behaviour.

Under qemu-aarch64 the gates that ran on the disc host: `newgame_to_field64` (30,500 flips, 24 flips/s, field 7:2, MAD 0.5509 %), `fields_gate`, `retail_save_gate`, `battle_gate`, `camp_menu`,
`save_load`, `title_mad`, `field_mad` and the sky scene pass; `story_gate`, `save_play`, `systems_gate club` failed exactly as they did on x86_64 then (the walk's `skipmsg` timed out; the new-game
MAD of the club floor was 33 %): 64-bit-model bugs and not AArch64 ones (the x86_64 build passes them now; they are hours under qemu and were not rerun there). `tools/p4pc_ci_run.sh` scope `a64` (the x64 scope cross-built for AArch64 in the `p4pc-ci-a64` image,
test time limits x4, two tests at a time, the tests labelled `slow` left out) runs them on a host whose binfmt runs AArch64 binaries; it is not in the land check (a cold run takes about 35
minutes on an idle host and over an hour with the llvmpipe scene gates on a loaded one; the five field scenes were what timed out) but one of the jobs of `tools/nightly_gates.sh`
(`P4PC_NIGHTLY_A64_HOST`, default a Linux host, the host that has the image: `docker build -t p4pc-ci-a64 -f ci/Dockerfile.a64 <context>` builds it on top of p4pc-ci, and `qemu-user` registered
in binfmt_misc with the F flag is what lets the container run an AArch64 binary directly). Its first full result (25 of 31 gates; near_fog read 72 % until the compiler runtime became a host
library, see CMakeLists.txt; the original full-history sky, riverbank and arcana viewer runs exceeded their 2400 s limits).

Natively on AArch64 (2026-11-05, GitHub's `ubuntu-24.04-arm` runners, `ci/Dockerfile.arm64`: Debian trixie arm64 with the same clang 19 and CMake as `p4pc-ci-a64`, the i386 headers and libraries
for the layout oracle and the name tables): the same CMake configuration builds on the machine's own architecture (gcc's files from `/usr/lib/gcc/aarch64-linux-gnu/14` instead of the cross
install), the census equals `config/port64a64_census.txt` (1206 units, clang 19), and the a64 scope's tests that need no game files pass: 175 of 214 selected, 39 skipped for the disc, build
9 minutes and tests under one minute on 4 cores (run 42088893499). `mod_hook_cost64`, red under qemu since 11-02, passes natively: its host-performance budget was qemu's. The land check and
the nightly run this job through the GitHub offload (docs/land_orchestrator.md); qemu-user on the a64 host confirms what it fails and runs the job when GitHub is unavailable.

The full-game clone loop labels the long workloads `slow`: `story_to_0415`, `street_0417`, `castle_gate_0417`, `castle_2f_0417`, `home_0417`, `save_play`, `fields_load`, `newgame_to_field`, `systems_club_floor_menu`, `scene_tv_world_enter` and `scene_shadow_haze`. The two castle walks contain 90,000 and 110,000 flips; the real-play TV-world and shadow-haze captures contain 75,594 and 110,560, so their clones use the same policy. Native x86 scopes continue to run them. The shorter `voiced_event_0417`, rain, riverbank, arcana and near-fog scenes stay in the default AArch64 nightly; a slow label is not a substitute for diagnosing a failing gate.
Only the AArch64 `scene_sky_black_tint64` viewer twin retains `slow`: it keeps all 600 rasterised field frames for feedback history. The original viewer versions of riverbank and arcana had the same classification, but their canonical real-play replacements use a 150-frame raster window and run in the ordinary AArch64 nightly. Native x86_64 coverage remains ordinary. This cutover does not relax a MAD limit; the real-play gates also check the captured guest state against retail.

The AArch64 fourfold CTest timeout is also forwarded through the existing `P4PC_GATE_TIMEOUT` environment setting. `save_play_gate.sh` and the event scenes that call it otherwise keep their native 1300-second inner watchdog: both the voiced event and canonical fog were killed with status 137 while still progressing, before their longer CTest budget expired. Native default deadlines and image/state limits are unchanged.

**Riverbank: isolate the viewer's boot lifecycle, not the GPU.** The full-history AArch64 capture had 258 draws at flip 1198 against x86_64's 211. Guest RAM's live task lists contained an extra root `titleDrawProc` at 0x00d6c830 (work 0x01051960): 56 tasks against 55 on x86_64 and the phone. All three sequence managers already said sequence 9, so checking the current sequence alone missed it. Draws 197..243 were the title's opaque black screen, background and letterbox/UI, inserted over the field; 204 of the original 211 normalised draw lines matched exactly, and the other seven differed only in printed one-pixel bounding boxes.

`render_field` called `kwlnInitSubsystems` (which starts the normal memory-card/boot sequence), manually started the boot's data-load callbacks again, then waited a fixed 600 frames. Deferred boot destruction callbacks could still launch the logo, movie and title. A fast host captured before that separate root task covered the field; a slow one did not. The original all-black frame is [INFERENCE] an earlier logo/fade phase of the same surviving boot flow. The viewer now waits for the real boot chain to reach sequence 27 with no delayed start pending, stops its separate logo root and `gameSequence` through the same seams the menu harness uses, waits for the manager to become idle, and only then starts FIELD RECORD or the requested field-step task. The duplicate boot callbacks and the per-frame hack that cleared pending-sequence flags are removed.

Matched-source post-fix captures (`5c1e8839`) both finish at flip 731 with 211 draws, 55 live tasks, identical task names/parent edges and no boot/logo/title root: x86_64 MAD 8.6105%, AArch64/qemu MAD 8.5072% (limit 8.8). The full AArch64 render takes 1148 s, against 3232 s in the contaminated diagnostic. The same 204 normalised draw lines match, with only the seven printed bounding-box differences remaining. Three camera-matrix words differ by one float ULP (right.x, up.x and pos.x from decimal parsing under the host FP control); the matrix flag words match. The mirror already uses signed `char`, and its AArch64 `va_list` struct-address rewrite is in place: no signedness, varargs or float-to-int workaround was added for this failure.

These full-history viewer captures qualify the boot-lifecycle correction independently of the later canonical real-play scene replacement; their MADs are not measurements of the replacement gates.

The combined phone build's full-history riverbank is 8.6183%; its sky and arcana controls are 2.6015% and 13.2699%. No MAD limit or rasterised-history window was relaxed for these checks.

**The battle floor gate** (`scene_battle_floor_a`, with a 64-bit twin `scene_battle_floor_a64`) read 15.18 % on i386 and 15.84 % on x86_64 and crashed outright on x86_64 at first. Three causes:
the effect kind setup allocators (`btlEplAllocColorPair`, `btlEplAllocColorValueTarget`, `btlEplAllocColor`: `void` in the decomp, returning the allocation in $v0 on the PS2, whose result
`EffBattleCreateFromRecord` stores at work+0x30) left a host pointer in rax and the effect stored its low half (a SIGSEGV in `btlEplStepColorPairFade`; mirror_fixes rows now make them
return the pointer); the indirect-call adapter pool of 256 was exhausted by the battle (1024 now); and the difference between the builds, found by diffing the RNG draws (`P4PC_LOG_RANDOM`: the
same values in the same order up to draw 629, which came one flip later on x86_64), then the packet tables (the wait conditions of `btlPacketStep`): the boss's first action waits for a packet whose
callback is `btlFormationHasEntry`, a `bool` function, and the wait condition reads the callback's result through a `s32 (*)(u8 *)` pointer. On i386 the function left false in AL and a stale
pointer above it (EAX 0x01425e00), so the condition read true at once and the action started 31 frames early; on x86_64 RAX was clean and the packet waited as on the PS2. The
narrow-return pass of `tools/gen_port.py` that should cover this (`widen_narrow_returns`) had been dead since a second pass defined the same regex names; it is now the bool-return pass
(the ten `bool` functions of btlCond.c and btlFormation.c return u32), and both builds follow the same timeline (the harness event streams are identical through flip 1400): 15.56 % on
i386, 15.46 % on x86_64 (14.24 % and 14.23 % once the fog battle's sector draw landed), limit 14.4. `P4PC_DUMP_RAM=<pattern with %u>` with `P4PC_CAPTURE_FRAMES` dumps the RAM at each captured frame and `tools/ram_diff.py --ram-ptrs` ignores heap
pointers (a 64-bit heap sits elsewhere); the data differences stay dominated by blocks allocated at other addresses, and the RNG log and the packet tables were what found this.

### 6.8 The host tests on x86_64

`tests/CMakeLists.txt` loads the numbered `tests/cmake/*.cmake` fragments in
lexical order, sharing the tests directory and CMake policy scope. Edit the
owning area fragment, not the loader. Areas may occur at several numbers:
the fragments preserve declaration order, target dependencies and final policy
passes rather than regrouping commands. New native reference fixtures belong before
`290-port64-host-cases.cmake`, which registers the cases and finishes both build aggregates.

Every executable of tests/CMakeLists.txt that is not the game has a `<exe>64` twin and every test that runs it a `<name>64` twin (label `port64`, so the land check's
x64 shard runs them; the i386 originals run in the nightly). Three recipes share the `P4_PORT64_LINK` block in `tests/cmake/220-port64-host.cmake`:

The applicable Windows twins use `<exe>64w` and `<name>64w`, label `port64win`,
and the `p4_x64w` aggregate. They run through the same 64-bit Wine prefix, after
the real coroutine test initializes it. The Wine scope builds GL witnesses but
does not run them, just as the Win32 scope does; Linux runs them, and the Windows
renderer is gated on the actual GPU. The two `ovdiff` interpreter cases are
POSIX-only and have no Win32 variant to twin.
The Luau and mod-binding host witnesses are declared outside `tests/`; the
`p4_x64` and `p4_x64w` aggregates also depend on their corresponding checker,
sandbox and mod interpreter executables whenever those providers are enabled.

- **The same sources for x86_64 and Win64** (`p4_host64_exe`, and `p4_host64_tests` called after every reference fixture and its final policy). Arguments, environment,
  working-directory requirements and limits are retained; outputs get an ABI prefix (`64_` or `64w_`), with private default working and save directories. This covers
  the CRI fixtures (`cri_*`, `adxf_partition`, `mwply`, `sfmpvp`), `kernel_threads`, `sdr`, `sebank`, `files_disc`, `native_tex`, `fpbit`, `config`, `p4iso`, `vag`, `pad_*`, `walk_tasks`, `vu0_helpers`, `mixer_device_*`,
  `movie_audio`, `ffmpeg_license`, `gl_cache`, `fpu_state`, `sfd_decode`, `sfdview_disc`. They are guest-safe by construction (tests/guest_test.h: the fixtures map the guest
  block on a 64-bit host and give storage the code reaches through a guest address). An exe whose source list names the generated tables or the mirror objects
  (`rofs_open`) links the selected ABI's tables and objects. Other object libraries are rebuilt for that ABI, preserving their compiler launcher
  (including the EE float launcher); original link options preserve live-boundary wraps. The OS source and SDL/FFmpeg imports use the selected platform. Existing twins (`render_smoke64`, `adx_wav64`, `ovdiff64`, ...) are reused with
  the other tests' arguments: `render_smoke_gl33/es30`, `adx_stream_*`, `movie_task_opening/p4ct001`, `ovdiff_dblcores`.
- **The retail override tests** (`spline`, `bmp`, `rxheap`, `rtanim`, `matrix_override`, `palquant`, `promoted_leaf`) compile the override sources against stubs and memory
  at the retail globals. On i386 the override is linked as it is; on a 64-bit host the shipped override is a rewritten piece of a mirror unit (guest-memory globals,
  4-byte guest pointers, the register ABI), so both twins compile the same source through `tools/port64/unit64.py build --src`, with its normal layout oracle,
  EE float launcher, rewrite rules, dependency files and pointer-conversion errors. The same test source works on i386 and both 64-bit targets: mirror calls run on a guest stack,
  guest-addressed buffers come from that stack or `p4t_galloc`/the guest fixture heap, and pointer words use `P4_H2G`. Native callback slots carry explicit
  `p4_fn_h2g_sig` signatures, as the production memory table does; retail globals use `P4_G2H(addr)` after `p4t_guest_map` (tests/ovr_guest.h has the allocator slots).
  These run the very functions `ovdiff_subset64` compares with the retail asm, but also the cases the interpreter cannot reach (error paths, hand-worked values).
- **`ee_libm`**: the retail libm cores through the EE float launcher, the same hashes as i386.

The only architecture-specific exception is AArch64's `fpu_state` (the x86 MXCSR check); both default x86_64 product ABIs cover every applicable host witness.
A new host test needs no registration: it gets its twin from the loop, so it must be guest-safe (no raw retail address cast to a pointer, no host pointer stored into
a guest word, no `uintptr_t` casts of guest data), and the build says so by failing to link or by a P4_GUEST_CHECK fault.

The original host case list remains in the `i386` nightly job (scope `main`) of
`tools/nightly_gates.sh`.

The audit baseline `f52e368` had 52 i386-only CTest cases, representing 42 new
executable builds (some cases share an executable, and eight are additional
variants of an already-64-bit executable). Comparing the reference case names
with their `64` twins gives this inventory; later fixtures use the same factory:

```text
adx_stream_env, adx_stream_event, adx_stream_voice, adxf_partition, bmp, config,
cri_acs, cri_acs_step, cri_acssnd, cri_adxm, cri_adxt_start, cri_cvfs, cri_init,
cri_stm, ee_libm, ffmpeg_license, files_disc, fpbit, fpu_state, gl_cache,
kernel_threads, matrix_override, memcard_roundtrip, mixer_device_dummy,
mixer_device_pacer, movie_audio, movie_task_opening, movie_task_p4ct001, mwply,
native_tex, ovdiff_dblcores, p4iso, pad_gamepad, pad_isolation_script,
pad_isolation_virtual, pad_keyboard, palquant, promoted_leaf, render_smoke_es30,
render_smoke_gl33, rofs_open, rtanim, rxheap, sdr, sebank, sfd_decode,
sfdview_disc, sfmpvp, spline, vag, vu0_helpers, walk_tasks
```

## Reproducing the numbers

All tools take an existing i386 build directory (`cmake -S . -B b -G Ninja -DCMAKE_C_COMPILER=clang -DP4_DECOMP=decomp -DP4_RETAIL=...; ninja -C b p4_mirror`) and need clang 19 with x86_64 and i386 headers (the p4pc-ci image has them).


```sh
ninja -C b p4decomp64                                  # 2.3: the 64-bit mirror, one .64.o per unit (b/gen/obj64)
ctest --test-dir b -R port64                           # port64_mirror (no unsupported construct), port64_task, port64_task_retail
python3 tools/port64/inventory.py b out/inv            # 1.2, 1.4: warnings, layouts, port src
python3 tools/port64/census.py b out/census --abi b/gen/obj64/abi.json --jobs 8   # 1.3: rule sites
python3 tools/port64/rewrite_all.py b out/ra --jobs 8 --abi-cache                 # whole-mirror summary without CMake
```

Files: `include/p4pc/guest.h`, `src/platform/guest.c` (block, function table), `p4_os_map_anywhere` in `include/p4pc/os.h`, `tools/port64/gptr_rewrite.py` (the rewriter), `unit64.py` (the per-unit steps CMake runs), `build64.py` (builds a 64-bit executable from one mirror unit), `rewrite_all.py`, `census.py`, `inventory.py`, `tests/port64_task_test.c`, `tests/port64_task.sh`, `tools/mipsdiff/r5900.c` (guest-block aware).
