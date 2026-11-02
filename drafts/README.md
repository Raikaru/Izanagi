# Romwright machine drafts (asm-only worklist)

Unreviewed machine drafts of the retail functions listed as `asm-only` in `docs/asm_worklist.md`
(432 rows, one file per row; the file name is the 8-digit address).

- `<addr8>.c`: first line is `/* <addr> <name> <unit> - romwright draft, unreviewed */`, then the
  romwright decompile output for that address, unchanged.
- `INDEX.md`: address, name, unit, line count of the file, and flags.
  Flags: `M` = the draft contains `halt_baddata` or `Bad instruction`; `W` = the draft contains `in_vf`.

They were made with romwright, a private decompiler that is not part of this repository, per address batch of 16.

Each file is the output for one address. These are drafts, not matches: they have no prototype from the
decomp and have not been checked against the asm.

Fix list:

For asm-only functions, start from a draft, never from scratch, and check it against the asm. Use the romwright draft (r5900 language) first. It decodes the EE FPU accumulator ops (`madd.s`/`msub.s`/`adda.s`/`mula.s`) and VU0 macro ops into plain float math, where m2c emits `M2C_ERROR`. Fix its 64-bit GPR types (`unsigned long long` params are 32-bit values), the `in_vfNw` w-lane reads, and missing call arguments carried in registers, then apply the decomp's prototype. The old m2c draft from decomp history is a second opinion for names and types: `git -C decomp show e5e04329^:src/generated/<unit>.c`.
