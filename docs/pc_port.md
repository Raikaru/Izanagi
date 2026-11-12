# PC port: platforms and word size

The phase-0 plan and its measurements live in the decomp ([`decomp/docs/port/README.md`](https://github.com/Raikaru/Persona4-Decompilation/blob/main/docs/port/README.md), a pinned submodule that this repo never edits). This page records the decisions made since, where they differ from it. Read the two together; where they disagree, this page wins.

## Baseline: five platforms

The port targets **Android, iOS, macOS, Linux and Windows**, each on 64-bit CPUs (arm64 and x86_64). The phase-0 plan said "Windows + Linux, 32-bit x86 first, 64-bit later"; the baseline changed, and "later" is now.

| Platform | CPUs | Notes |
|---|---|---|
| Linux | x86_64 (i386 today), arm64 | clang; SDL3; today's shipping build is i386 |
| Windows | x64 (i386 today), arm64 | llvm-mingw; the `.ps2ram` PE-section trick is i386-only and goes away on x64 |
| macOS | arm64, x86_64 | Apple's kernel never maps below 4 GB: no fixed guest address (see below) |
| Android | arm64 | bionic has no `ucontext`; the thread model needs its own context switch. The 32-bit x86 build already runs on the Android emulator ([android.md](android.md): SDL3 activity, document picker, touch pad, context loss, the scene gates), which proves everything Android-specific before arm64 |
| iOS | arm64 | no JIT, no `ucontext`, no fixed low mappings |

What stays out of this page: the renderer API (OpenGL ES 3.0 by default, desktop OpenGL 3.3 core selectable; Apple platforms run the ES path through ANGLE, with the cost and the SDL3 GPU API alternative evaluated in `docs/renderer_port.md` section 10, owned by the renderer lane) and packaging (app bundles, APK/AAB, signing).

### What the baseline forces

1. **No fixed guest address.** The i386 build maps the PS2's RAM at its retail addresses and uses PS2 pointers as host pointers. That cannot work on a 64-bit host that also has to run on Apple platforms, where a 64-bit process cannot map below 4 GB ([Wine on macOS](https://www.winehq.org/pipermail/wine-devel/2020-August/171857.html), [WineConf 2022 talk](https://indico.freedesktop.org/event/2/contributions/92/attachments/87/140/Wine%20on%20macOS%20State%20of%20the%20Union%20-%20WineConf%202022.pdf)). Guest RAM is one allocated block; a PS2 pointer in guest memory stays a 4-byte guest address and is translated on access.
2. **64-bit word size** with struct layouts still PS2-shaped (retail data, saves and asset streams carry 4-byte pointers).
3. **A portable coroutine layer** (no `ucontext` on Android or iOS; Windows fibers own their stacks, which breaks "stacks live in guest memory").
4. **Float mode per CPU**: the EE's round-toward-zero and flush-to-zero are MXCSR on x86 and FPCR on arm64.

All of it is designed, measured and staged in [port64.md](port64.md). That page also holds the model for new code; the working rule is in [CONTRIBUTING.md](../CONTRIBUTING.md) ("64-bit from now on") and applies from now on to every lane.

### What does not change

- The i386 Linux and Windows builds keep shipping and keep every gate green while the 64-bit path is built beside them. Playability work continues on i386.
- The decomp is not edited from here. The 64-bit model is a build-time pass over the generated mirror (`tools/gen_port.py` territory), not a change to the decomp's C.
- Data still stays at its retail address, now as a guest address in the block.
- Overrides keep one owner per address (`overrides/CLAIMS.md`); an override is written once and built for both word sizes.
