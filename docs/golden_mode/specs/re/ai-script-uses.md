# Golden AI call-site evidence: historical Steam helper limits

**INFERRED (protected helper):** these observations describe static call-site
structure and do not assert protected helper semantics, branch reachability,
or the runtime values of locals. Confidence is high for word offsets,
immediate arguments and the observed operations; low for gameplay meaning.
No protected constants were evaluated. The source is the instruction sections
of Steam build 10836033 `battle/AICALC.TBL` blocks 9 and 10. Offsets below are
zero-based instruction-word indices, not executable addresses. All listed
sites are in block 9; none of these three commands occurs in block 10. The
parameter slots follow the VM's top-of-stack-first convention (PS2
`scrGetParamInt` at `0x0029CC00`; Golden accessor `0x46AC20`).

The helper limitations below describe the Steam-only investigation, not the
current R4 status. The exhaustive pinned-Switch contract, including the
maximum-SP second sum, persona/cost dependencies, affinity-bit projection,
both RNG streams and seed period finalizer, is now **SPECIFIED** in
[ai-commands.md](ai-commands.md). Steam protected internals remain unproved;
this historical call-site evidence still makes no dynamic-reachability claim.

| Command | Word indices | Arguments observed at each call | Result use |
| ---: | --- | --- | --- |
| 8471 | 3234, 3240 | slot 0 = integer local 45; slot 1 = 0, then 1 | Either path stores its result to integer local 13, joins, and tests it against -1 before subsequent control flow. |
| 8471 | 3323, 3329 | slot 0 = integer local 46; slot 1 = 0, then 1 | Same local-13 storage and -1 test. |
| 8471 | 3408, 3414 | slot 0 = integer local 47; slot 1 = 0, then 1 | Same local-13 storage and -1 test. |
| 8473 | 11234, 11279, 11434 | slot 0 = 50, 30, 50 respectively | Each result is compared for equality with 1, followed by a conditional branch. |
| 8484 | 5320 | slot 0 = integer local 67 | The return and a separate zero-argument command's return participate in two integer subtractions; the result is stored in integer local 68. The next command is 8485 with local 67. |

The paired rows above describe alternative control-flow sites, not necessarily
calls that execute one after another. Commands 8471 and 8484 reach helpers
whose observed dependency chain includes `0xD6E80` → `0x1EE84BC0` and
`0xCFBD0` → `0x1EDE4B20`; 8473 reaches `0xCF790` → `0x1EDB6D30`. At the
last two targets, static entry bytes contain encoded constants; no attempt was
made to evaluate them. An ordinary control-flow entry is visible at
`0x1EE84BC0`, but its complete downstream effect remains unproven. See
[the dispatch specification](ai-commands.md) for the verified handler and
argument counts. These call sites do not justify inventing semantics.

The ordinary helper at `0x1EE84BC0` corresponds structurally to the PS2
`datPersonaGetRecord` accessor (`0x0010A900`): party ID 1 takes a selected
roster-record path, which returns the record only if its active flag is set;
other IDs tail-call the ordinary accessor at `0x19FF30`. The full effects
of the selected-record update helper and the protection-encoded downstream
eligibility path have not been established.

## Synthetic vectors

These are abstract instruction-structure checks, not extracted game code:

| Synthetic situation | Expected inference |
| --- | --- |
| 8471 called with top-of-stack local 45 and the next stack value 0 | slot 0 is local 45; slot 1 is 0 |
| 8473 result compared with integer 1 then conditionally branched | report a test for equality with 1, not an inferred HP/SP meaning |
| a call site lies in an instruction section but its procedure is never entered | do not infer dynamic reachability from its static presence |
