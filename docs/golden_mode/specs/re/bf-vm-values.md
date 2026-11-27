# Golden BF VM values and opcodes

## Status and provenance

**SPECIFIED for the pinned Switch executable**, with ordinary Steam corroboration and the explicit architecture differences below. This is a static clean-room contract: prose, address citations and synthetic vectors, not a transcription of the interpreter. It does not establish game execution, adapter correctness or a lifted boot gate.

| Requested area | Status | Important result |
| --- | --- | --- |
| Push/immediate extension | SPECIFIED | PUSHI sign-extends s32; PUSHIS sign-extends s16; indexed integer loads and PUSHREG preserve 64 bits. |
| Arithmetic and exceptional cases | SPECIFIED | **Binary integer operations are s32, not s64**, despite the wide carrier. MINUS and NOT use the full s64 value. No MOD opcode exists. |
| Promotion, comparisons, conversion | SPECIFIED | Mixed arithmetic promotes full s64 to binary32. Switch float-to-s64 saturates; Steam returns the x86 indefinite value for invalid conversion. |
| Local/global banks | SPECIFIED | Integer cells are eight bytes, float cells four bytes; signed s16 opcode indices; assignment conversion is explicit below. |
| Tags, calls and branches | SPECIFIED | Separate address/string tags; CALL saves the call-site PC. Integer IF tests **low32**, unlike NOT. |
| Capacity and return slot | SPECIFIED | 47 ordinary stack entries plus reserved entry 47; persistent native result, not an END-produced result. Switch diagnostic routines return without enforcing guards. |

Primary image: Switch update content version 131072, main NSO SHA-256 `854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`, build ID `66b2b6d53549a6364a43f69e282e3882f4d7028e000000000000000000000000`. All `0x710...` citations are virtual addresses, base `0x7100000000`. Steam citations are **RVAs**, ordinary `.shared` only, build 10836033. No `.arch` target was decoded or followed. Keys were not accessed; neither game nor emulator was run.

Opcode names and numbering follow TGE's [public opcode enumeration, pinned at 05483fe12f9c482ee17f38ded2e115f4c55015b3](https://github.com/tge-was-taken/Atlus-Script-Tools/blob/05483fe12f9c482ee17f38ded2e115f4c55015b3/Source/AtlusScriptLibrary/FlowScriptLanguage/Enums.cs). Public descriptions are naming/format evidence, not a substitute for native behavior. In particular, native IF branches on **false**, JUMP uses the **procedure** table, and PUSHSTR stores a **pointer**, not an integer string index. The public enumeration also has POPREG=35; this Switch VM admits only opcodes **0–34**, so POPREG is not supported here. The older [assembly reference](https://github.com/tge-was-taken/Atlus-Script-Tools/blob/05483fe12f9c482ee17f38ded2e115f4c55015b3/Documents/Reference/FlowScriptAssemblyReference.txt) is not authoritative for NOT/OR/AND: these are logical operations, not bitwise complement/OR/AND.

## 1. Carrier and notation

Use `I(n)` for an integer-tagged 64-bit bit pattern interpreted as signed two's-complement, `F(x)` for a binary32 value, `A(pc)` for a return-address-tagged value, and `S(ptr)` for a string pointer. Stack diagrams run **bottom to top**. `s32(x)` means the signed interpretation of x's low 32 bits; `s64(x)` means modulo-2^64 signed interpretation. These are explicit bit operations, not undefined signed C overflow.

The Switch context has an s32 instruction-word PC at `+0x1C`, s32 stack depth at `+0x20`, byte tags at `+0x24`, and eight-byte payload slots at `+0x58`. Slot 47's tag is `+0x53`, and its payload is `+0x1D0`: that is the native result register. Float payloads occupy only a slot's **low four bytes**. Float pushes, arithmetic, assignment to the result register and float negation do not clear the high four bytes. They are not binary64 values, nor guaranteed zero-extended bit patterns (`0x71002E15E0`–`0x71002E1664`, `0x71002E1470`–`0x71002E1484`).

| Tag | Meaning | Payload |
| ---: | --- | --- |
| 0 | Integer | Full signed 64-bit value |
| 1 | Float | IEEE binary32 in low32; upper32 preserved/irrelevant to float readers |
| 2 | Global-integer reference | Full 64-bit index into the eight-byte integer bank |
| 3 | Global-float reference | Full 64-bit index into the four-byte float bank |
| 4 | Return address | Sign-extended s32 call-site word PC |
| 5 | String | Full native pointer into the script string section |

PUSHIX/PUSHIF produce **snapshots tagged 0/1**, not tags 2/3. None of the 35 opcode handlers constructs tag 2/3. Those reference cases are nevertheless implemented by readers and unary handlers, so a carrier must not conflate them with ordinary numbers (`0x71002E1670`, `0x71002E1700`, `0x71002E2940`). PUSHREG copies its stored tag without normalizing it.

### Numeric readers

The following rules distinguish conversion from later opcode-specific narrowing:

| Input tag | Integer pop / integer command argument | Float pop | Float command argument |
| --- | --- | --- | --- |
| 0 | Full s64 | Convert full s64 to f32 | Convert full s64 to f32 |
| 1 | Convert low f32 to s64, truncate toward zero | Low f32 | Low f32 |
| 2 | Load full s64 from global integer bank | Load full s64, convert to f32 | **Load low32 of the integer cell as signed s32, then convert to f32** |
| 3 | Load global f32, convert to s64 | Load global f32 | Load global f32 |
| 4 | Full address payload | Consume it and return F(+0) after a no-effect diagnostic | Convert full signed address payload to f32 |
| 5 / other | Return I(0) after a no-effect diagnostic | Consume it and return F(+0) after a no-effect diagnostic | Return F(+0) after a no-effect diagnostic |

An integer/float **pop consumes one slot**. Command accessors instead read slot `sp - 1 - argument_index`, leave depth unchanged, and use the process-global current context set by COMM. Argument index 0 is the topmost argument. The argument-index guard tests only signed `sp > index`; there is no negative-index lower-bound guard. A string command accessor accepts tag 5 only and returns the pointer; all other tags return null after the diagnostic. Valid callers must provide backed indices.

Evidence: integer assignment/pop `0x71002E1810`–`0x71002E1908`; float pop `0x71002E2940`–`0x71002E2A5C`; argument readers `0x71002E11C0`–`0x71002E1450`. The tag-2 float-argument exception is independently visible in the **32-bit** signed conversion at `0x71002E1390`, versus the **64-bit** signed conversion at `0x71002E29FC` for float pop. Do not generalize one accessor's behavior to all readers.

## 2. Complete opcode table

Instructions are little-endian four-byte words: signed s16 opcode followed by signed s16 operand. PUSHI/PUSHF consume the following whole word as their payload, advancing PC by two words. Other ordinary handlers advance PC by one unless they transfer control. In the table `k` is the signed s16 operand; `a` is the **first popped/top** operand and `b` the next. Binary handlers consume both and push one result. Pushes add one slot; assignments consume one.

Dispatch loop `0x71002E10D0`–`0x71002E11BC` selects the relocated 35-entry table at `0x71007AF218`. Its signed opcode checks precede dispatch, but their diagnostic functions do not stop execution on this build.

| Opcode | Name | Switch handler | Value/control effect |
| ---: | --- | --- | --- |
| 0 | PUSHI | `0x71002E1560` | Push I(sign-extend-s32(next word)); **not** zero extension, and not a 64-bit immediate. |
| 1 | PUSHF | `0x71002E15E0` | Copy next word's raw f32 bits to low32 of a tag-1 slot; no integer/float numeric conversion. |
| 2 | PUSHIX | `0x71002E1670` | Push full eight-byte global integer cell k as I. |
| 3 | PUSHIF | `0x71002E1700` | Push four-byte global float cell k as F; no widening. |
| 4 | PUSHREG | `0x71002E1790` | Copy reserved slot 47's tag and **all eight payload bytes** to the next ordinary slot. |
| 5 | POPIX | `0x71002E1810` | Integer-pop conversion, store full s64 to global integer cell k. |
| 6 | POPFX | `0x71002E1910` | Float-pop conversion, store four-byte f32 to global float cell k. |
| 7 | PROC | `0x71002E1960` | No value effect; advance one word. Does not allocate locals or reset result. |
| 8 | COMM | `0x71002E1980` | Native command dispatch/argument cleanup/yield rules in section 5. Does not itself push a value. |
| 9 | END | `0x71002E1B20` | Empty stack: stop. Otherwise integer-pop, take low32, add one modulo 2^32, install as PC. Expected tag is 4, but the check is non-enforcing. Does not write the result register. |
| 10 | JUMP | `0x71002E1C40` | Transfer to procedure-table record k's word PC; no saved address, no stack/result change. |
| 11 | CALL | `0x71002E1C70` | Push A(current PC), then transfer to procedure k. No automatic argument conversion/pop and no result write. |
| 12 | RUN | `0x71002E1D30` | Unsupported diagnostic, which returns on this build; then advance one word with no stack/result change. |
| 13 | GOTO | `0x71002E1D80` | Transfer to jump-label-table record k's word PC; no stack/result change. |
| 14 | ADD | `0x71002E1DB0` | a + b under section 3's integer/f32 selection. |
| 15 | SUB | `0x71002E1DF0` | a - b under section 3. |
| 16 | MUL | `0x71002E1E30` | a * b under section 3. |
| 17 | DIV | `0x71002E1E70` | a / b under section 3, truncating signed integer quotient toward zero. |
| 18 | MINUS | `0x71002E1EB0` | Tag 0: in-place **64-bit** modulo negate. Tag 1: in-place f32 sign negation. Tag 2/3: negate the referenced global cell in place, leave stack reference/tag/depth unchanged. Tag 4/other: diagnostic, leave value/depth unchanged. |
| 19 | NOT | `0x71002E1FD0` | Tag 0/2: integer-pop, compare **all 64 bits** with zero. Tag 1/3: float-pop, compare with floating zero. Push I(1) for zero, I(0) otherwise. Tag 4/other: diagnostic, no pop or replacement. |
| 20 | OR | `0x71002E21E0` | I(1) iff either selected numeric operand is nonzero, else I(0). Both operands are consumed; no source-expression short circuit. |
| 21 | AND | `0x71002E2220` | I(1) iff both selected numeric operands are nonzero, else I(0). Both consumed. |
| 22 | EQ | `0x71002E2260` | I(a == b), using selected numeric domain. |
| 23 | NEQ | `0x71002E22A0` | I(a != b). |
| 24 | S | `0x71002E22E0` | I(a < b). |
| 25 | L | `0x71002E2320` | I(a > b). |
| 26 | SE | `0x71002E2360` | I(a <= b). |
| 27 | LE | `0x71002E23A0` | I(a >= b). |
| 28 | IF | `0x71002E23E0` | Tag 0/2: integer-pop, branch to label k iff **low32 is zero**. Tag 1/3: float-pop, branch iff f32 equals either signed zero. Otherwise advance one word. Tag 4/other: diagnostic, **do not pop**, take label k. |
| 29 | PUSHIS | `0x71002E2580` | Push I(sign-extend-s16(k)). |
| 30 | PUSHLIX | `0x71002E2600` | Push full eight-byte local integer cell k as I. |
| 31 | PUSHLFX | `0x71002E2690` | Push four-byte local float cell k as F. |
| 32 | POPLIX | `0x71002E2720` | Integer-pop conversion, store full s64 to local integer cell k. Upper-bound diagnostic does not enforce bounds. |
| 33 | POPLFX | `0x71002E2860` | Float-pop conversion, store four-byte f32 to local float cell k. |
| 34 | PUSHSTR | `0x71002E28B0` | Push S(string-section-base + sign-extended-s16(k)), a byte offset, with full pointer width. |

The pivotal immediate and copy widths were cross-checked independently with llvm-mc at `0x71002E1588`, `0x71002E25A0`, `0x71002E169C`, `0x71002E1644`, `0x71002E17E0`, `0x71002E1CC0`. The code does not implement a zero-extending unsigned-immediate opcode. To obtain values outside s32, a script needs another producer, e.g. a native result or f32-to-integer assignment.

## 3. Binary arithmetic, comparisons and exceptional values

### Integer path: deliberately narrower than the carrier

The shared operator at `0x71002E2A60` selects the integer path **only if both original tags are 0 or 2**. It integer-pops a and b, saves only each value's low32 into signed 32-bit temporaries, and operates in that domain. ADD/SUB/MUL retain low32; DIV uses signed 32-bit division. The result is sign-extended from s32 to the eight-byte I carrier. Integer comparisons are signed s32; integer OR/AND test the truncated operands. **There is no 64-bit binary-integer arithmetic path.**

Evidence: first/second temporary stores `0x71002E2D74` / `0x71002E2E34`; ADD/SUB/MUL `0x71002E2E64`, `0x71002E2E88`, `0x71002E2EA0`; signed division `0x71002E2ED0`; result sign extension `0x71002E2F70`. Unary full-width negate at `0x71002E1F18` and full-width zero test at `0x71002E2148` are intentionally different. Integer IF's low32 zero test is `0x71002E2494`.

| Case | Switch result | Steam ordinary `.shared` corroboration/difference |
| --- | --- | --- |
| Integer ADD/SUB/MUL overflow | Wrap modulo 2^32, then sign-extend | SUB `0x46A0F8`–`0x46A106`, MUL `0x46A1CB`–`0x46A1D2` corroborate 32-bit arithmetic and signed extension passed to the push helper. ADD itself is protected; no separate Steam ADD proof. |
| DIV divisor low32 == 0 | Diagnostic returns; signed hardware division yields I(0) | `0x46A29B`–`0x46A2A2` uses 32-bit x86 IDIV: zero divisor raises native #DE, not I(0). |
| DIV a low32 == INT32_MIN, b low32 == -1 | I(-2147483648), no trap | Same Steam IDIV raises #DE for quotient overflow. |
| DIV a == INT64_MIN, b == -1 | a narrows to 0, so I(0) | Same low32 inputs are 0/-1; no divide overflow at this edge. |
| MINUS INT64_MIN | I(INT64_MIN), modulo 2^64 | Full-width NEG corroborated at `0x46A3CA`–`0x46A3D2`. |
| NOT I(2^32) | I(0), because full64 is nonzero | Full-width zero test corroborated at `0x46A448`. In contrast, integer OR with zero yields I(0), and IF takes its false label. |

This is not permission to substitute host signed C overflow or host division exceptions. A mode that targets Steam's invalid-script edges must preserve its native divide-fault distinction, rather than claiming the Switch result is cross-platform Golden behavior.

### There is no MOD opcode

The 35-entry dispatch set contains no remainder operation. Do not invent a wide MOD VM instruction or infer a special divide-by-zero remainder rule. A source compiler can synthesize remainder from DIV/MUL/SUB; its behavior is exactly that emitted sequence. For the explicit synthetic expansion in section 6, Switch's result is `s32(a - s32(truncdiv32(a,b) * s32(b)))`; zero divisor gives s32(a), and INT32_MIN/-1 gives 0. On Steam, the expansion faults at its DIV for zero divisor or INT32_MIN/-1. This specifies the sequence, not every possible compiler lowering of a source-language `%`.

### Float/mixed path

Any other tag combination selects the float path. Each operand is float-popped **in full carrier width before conversion**: an integer I(2^32+1) is converted from s64, not first narrowed to s32. Tags 4/5/unknown contribute F(+0) through the diagnostic/default path described earlier, rather than pointer comparison or string parsing. This fallback consumes them. MINUS/NOT/IF have their own invalid-tag behavior and must not reuse this binary fallback blindly.

ADD/SUB/MUL/DIV each perform one binary32 operation and return F. Integer-to-f32 conversion and the operation round separately under the active floating-point control state. There is no f64 intermediate and no fused multiply/add. Floating DIV's zero-divisor diagnostic also returns on Switch; ordinary IEEE results follow (e.g. 1/+0 = +infinity, 0/0 = NaN under nontrapping controls). Signed zero, infinity and NaN remain float values; arithmetic overflow is floating infinity, not integer wrapping.

Comparisons and logical OR/AND produce I(0) or I(1), never F. For either NaN operand: EQ is 0, NEQ is 1, S/L/SE/LE are all 0. Both signed zeros compare equal. NaN is nonzero for logical truth, so NOT NaN = 0 and float IF NaN falls through. Logical OR/AND pop and convert both operands even when one truth value already decides the result. This is distinct from converting a float to integer first (e.g. F(0.5) is true, not false).

Evidence: binary float selector/operations `0x71002E2AA8`–`0x71002E2D08`; equality/inequality `0x71002E2C70`–`0x71002E2C90`; ordered comparisons `0x71002E2CA4`–`0x71002E2D00`; NOT `0x71002E2088`–`0x71002E20BC`; IF `0x71002E249C`–`0x71002E24A8`. Steam SUB/MUL/DIV float paths at `0x46A11F`–`0x46A156`, `0x46A1EE`–`0x46A224`, `0x46A2C3`–`0x46A2FA` corroborate f32 operations. Steam EQ/NEQ explicitly handle unordered at `0x46A697`–`0x46A6A4` and `0x46A754`–`0x46A761`; S/SE at `0x46A80F`–`0x46A820`, `0x46A98F`–`0x46A9A0` require ordered comparison. Steam NOT's parity/zero handling at `0x46A42C`–`0x46A43A` corroborates NaN-false zero equality.

### Float to integer is architecture-specific

All integer-pop/argument/assignment conversions use **signed 64-bit** float conversion first. There is no intermediate s32 cast except where the consuming opcode subsequently truncates. Switch's conversion at `0x71002E126C`, `0x71002E18D8`, `0x71002E2820` truncates toward zero independently of the FP rounding mode and saturates at signed-64 bounds:

| f32 input | Switch s64 | Steam s64, masked invalid exception |
| --- | ---: | ---: |
| Finite, truncated result representable | Truncate toward zero | Same |
| NaN | 0 | INT64_MIN |
| +infinity or positive overflow (including +2^63) | INT64_MAX | INT64_MIN |
| -infinity or negative overflow | INT64_MIN | INT64_MIN |
| Exactly -2^63 | INT64_MIN, valid | INT64_MIN, valid |
| Either signed zero | 0 | 0 |

Steam integer pop `0x469417`–`0x46945F` and integer argument accessor `0x46AC69`–`0x46ACB6` use 64-bit CVTTSS2SI. For the latter, invalid/out-of-range conversion's integer-indefinite result is `0x8000000000000000`; unmasked exceptions would instead interrupt native execution. For Switch positive overflow followed by integer ADD, the saturated INT64_MAX is **then** narrowed to -1; preserving it through a local assignment does not narrow it.

The VM does not configure FPCR itself. The whole pinned text census found no FPCR-writing instruction; this is not a proof of inherited process/thread control state. Ordinary-value vectors below assume round-to-nearest/ties-to-even, nontrapping arithmetic, and no flush-to-zero. NaN payload selection/quieting and subnormal handling follow the relevant native FP control state/instruction, not an invented canonical NaN rule. On Steam the corresponding controls are MXCSR.

## 4. Banks and indexing

| Bank | Cell/assignment | Index and backing |
| --- | --- | --- |
| Global integer | Eight-byte s64; POPIX uses integer-pop conversion, no s32 assignment truncation | Signed s16 operand scaled by 8; GOT `0x7100881098` relocates to native BSS bank base `0x7104A89390`. |
| Global float | Four-byte f32; POPFX uses float-pop conversion | Signed s16 operand scaled by 4; GOT `0x71008810A0` relocates to bank base `0x7104A89420`. |
| Local integer | Eight-byte s64; POPLIX uses integer-pop conversion | Context `+0x230`; signed s16 operand scaled by 8. |
| Local float | Four-byte f32; POPLFX uses float-pop conversion | Context `+0x238`; signed s16 operand scaled by 4. |

Global and local banks are distinct, not alternate integer/float views of one array. Pushes copy current cell contents, so a later assignment does not retroactively change an already pushed value. Local banks belong to the script context, **not a fresh CALL activation**: neither CALL nor PROC allocates/clears them. Script startup's diagnostic local-count limits are 800 integers and 8 floats (signed halfwords at header `+0x14/+0x16`), but its diagnostics do not reject excessive counts on this build. It supplies a pooled backing block with the float bank `0x1900` bytes after the integer bank (`0x71002E0548`–`0x71002E0650`): 800 eight-byte integer cells followed by eight four-byte float cells. Startup does not separately clear that pooled local block at checkout; do not infer zero initialization of recycled local values. Stack/result initialization is separate.

Ordinary local reads and float writes have no per-index bound check; POPLIX has only an upper-bound diagnostic against the declared integer count, no lower-bound guard. The three diagnostic routines do not enforce that bound on this Switch image. Global accesses likewise do not check the operand against a bank length. The inspected VM does not supply a safe global extent/clamp rule: **do not infer Steam's allocation extent from the Switch addresses, or manufacture zero for out-of-bank reads**. Invalid/negative indices access native adjoining backing; their numeric result is not determined by opcode semantics alone. A clean-room adapter must retain required backing or reject unsupported malformed input explicitly, not claim a fabricated value is retail behavior. Reference tags 2/3 use their full64 payload index, unlike the signed16 opcode operand.

Steam ordinary PUSHIX/PUSHLIX at `0x469613`–`0x469624` / `0x4696B3`–`0x4696C4` corroborate signed16 indexing and eight-byte integer loads. Global float reads at `0x469640` and local float reads at `0x4696E0` use four-byte cells. Local assignment `0x469900`–`0x46994E` corroborates the non-enforcing upper-bound check and full64 store. Steam's global integer/float bases are `0x1E415E60` / `0x1E416660`; these are **data addresses**, not permission to decode `.arch` instructions. Only their ordinary `.shared` consumers were decoded.

## 5. Control flow, result lifetime and capacity

### Calls and command arguments

Procedure and jump-label records have 32-byte stride and an s32 word PC at record `+0x18`. CALL saves the **current** s32 PC, sign-extended to s64 with tag 4, then uses signed16 procedure operand k. END ordinarily pops that A and resumes at saved-PC+1. JUMP is a tail procedure transfer, whereas GOTO/IF select the separate jump-label table. None uses a numeric stack item as a destination operand (`0x71002E1C40`–`0x71002E1D24`, `0x71002E1D80`–`0x71002E1DA0`).

END's expected-address check is only diagnostic. On this Switch build, a nonempty stack with another tag still goes through integer-pop conversion and low32 PC+1. Thus a float return address is converted to s64 then narrowed, a global reference is dereferenced, and a string/unknown tag returns zero so PC becomes 1. This is malformed control flow, not an alternate script return-value convention. On a valid empty-stack END, the loop stops and does not push or clear a return value.

COMM interprets its signed16 operand as a packed command: group `(sign-extended operand >> 12) & 0xFF`, index `operand & 0xFFF`. It selects 16-byte group/command records, sets the current-context pointer and invokes the command with no BF stack conversion. Argument accessors implement the conversions above. A zero native command success flag returns the interpreter's yield code 2: **no automatic argument pop and no automatic PC advance**. A nonzero flag subtracts the descriptor's low32 argument count from depth, then increments PC only if the handler did not already change it. The BF result register is independent of that native success flag. These rules are corroborated by Steam `0x4699E0`–`0x469A81`.

The loop (`0x71002E10D0`) continues on handler result 1, stops on 0, and returns a suspended state on 2. The loop's outward stop result is 2, outward yield result 1. Do not confuse these two levels of status with a script integer result. Yield updates timing fields; it does not clear the reserved result. Unsupported RUN still advances on the pinned Switch because its diagnostic returns.

### Result and stack storage

There are **48 physical payload slots**, with ordinary capacity **47 entries (indices 0–46)**. Reserved slot **47** is the native result register. All pushes, including CALL's saved address, use the same ordinary capacity. Address depth is not a second call stack. A push from depth 46 succeeds and leaves depth 47; another push reaches the overflow diagnostic.

The pinned diagnostic functions at `0x7100349960`, `0x7100349970`–`0x7100349998`, `0x71003499A0` **return without logging, halting or changing VM state** (the variadic routine only spills/restores its native argument area). Consequently this image's overflow/underflow/index/tag diagnostics are not traps. At depth 47, a push proceeds to **overwrite reserved slot 47**, increments depth to 48, and can change the native result. At depth 48, a further payload write starts at `+0x1D8`, corrupting the following context field. This is native corruption, not an extra supported stack slot or a well-defined growable-stack feature. Underflow and unbacked indices likewise remain unchecked memory accesses; no safe underflow zero rule exists.

A new context starts with depth 0, all 48 payloads zero and all tags zero, including result I(0), at `0x71002E04E0`–`0x71002E053C`. Native integer result setter `0x71002E1450` writes tag 0 and all64. Float setter `0x71002E1470` writes tag 1 and low32 only. Neither pushes. PUSHREG copies the current tag/full64 payload and increments depth. Native void commands leave the prior result intact; COMM, CALL, PROC and END do not implicitly clear it. Steam setters `0x46ADA0`–`0x46ADCC` and `0x46ADD0`–`0x46AE04` corroborate integer64 versus float-low32 stores; Steam float/string/address push helpers `0x4692A0`, `0x469310`, `0x469370` corroborate the 47-entry guard and tag/payload layout.

### Invalid tags, summarized for the pinned Switch

| Consumer | Tag 4 | Tag 5 / unknown |
| --- | --- | --- |
| Integer pop / integer argument | Accept full address as integer | Diagnostic then integer 0 |
| Float pop | Pop, diagnostic, float +0 | Pop, diagnostic, float +0 |
| Float argument | Convert full signed address to f32 | Diagnostic, float +0, no pop |
| String argument | Null, no pop | Tag 5 pointer; unknown null, no pop |
| Binary operator | Forces float path; contributes +0 | Forces float path; contributes +0 |
| MINUS / NOT | Diagnostic, unchanged slot/depth; advance | Same |
| IF | Diagnostic, retain slot, jump false label | Same |
| END, nonempty | Expected address; pop and use low32+1 | Diagnostic then pop; string/unknown resolves to PC 1 |

These default paths are explicitly observed, not a recommendation to accept malformed BF silently. Steam protected IF/END handlers have not been independently established at these malformed edges.

## 6. Synthetic vectors

These are **derived expectations**, not runs of retail instructions or the game. `ret=I(n)` / `ret=F(x)` means a synthetic native result setter has seeded slot47. `LI`, `LF`, `GI`, `GF` are backed local/global banks; a named label/procedure fixture has the PC stated. F32 bit patterns are hexadecimal. Except capacity/invalid-tag vectors, depth stays inside 0–47 and indices are backed. Every binary sequence pushes **b before a**.

### Push and bank vectors

| Initial state / opcode sequence | Expected stack/state |
| --- | --- |
| `PUSHI 0xFFFFFFFF; PUSHIS 0xFFFF` | `[I(-1), I(-1)]`; both sign-extend, PC advances 3 words. |
| `PUSHI 0x80000000; MINUS` | `[I(2147483648)]`, not INT32_MIN; MINUS is wide. |
| `PUSHIS 0x8000` | `[I(-32768)]`. |
| `PUSHF bits(0x80000000)` | `[F(-0)]`; upper32 of destination slot unchanged. |
| `ret=I(0x123456789ABCDEF0); PUSHREG; POPLIX 0; PUSHLIX 0; POPIX 0; PUSHIX 0` | `[I(0x123456789ABCDEF0)]`; LI0 and GI0 contain all64, result unchanged. |
| `PUSHF 4294967296; POPLIX 0; PUSHLIX 0` | `[I(4294967296)]`; assignment did not narrow to s32. |
| `ret=I(16777217); PUSHREG; POPLFX 0; PUSHLFX 0; POPFX 0; PUSHIF 0` | `[F(16777216)]` under RN-even; LF0/GF0 both that f32. |
| `GI0=1; PUSHIX 0; PUSHIS 2; POPIX 0` | `[I(1)]`, GI0=2: push was a snapshot, not a reference. |
| String section base B; `PUSHSTR 3` | `[S(B+3)]`; string argument 0 returns B+3; integer argument 0 returns 0; neither accessor pops. |
| Backing explicitly supplies GI[-1]=9; `PUSHIX 0xFFFF` | `[I(9)]`, not an access to GI[65535]. Without that backing, no numeric result is promised. |

### Arithmetic and promotion vectors

| Initial state / opcode sequence | Switch expected result |
| --- | --- |
| `PUSHIS 1; PUSHI 2147483647; ADD` | `[I(-2147483648)]`. |
| `PUSHIS 1; PUSHI -2147483648; SUB` | `[I(2147483647)]`. |
| `PUSHI 65536; PUSHI 65536; MUL` | `[I(0)]`. |
| `PUSHIS 3; PUSHIS -7; DIV` | `[I(-2)]`. |
| `PUSHIS 0; PUSHIS 7; DIV` | `[I(0)]`; Steam native #DE instead. |
| `PUSHIS -1; PUSHI -2147483648; DIV` | `[I(-2147483648)]`; Steam native #DE instead. |
| `ret=I(INT64_MIN); PUSHIS -1; PUSHREG; DIV` | `[I(0)]`: numerator narrows to 0, not a signed64 overflow division. |
| `ret=I(INT64_MIN); PUSHREG; MINUS` | `[I(INT64_MIN)]`. |
| `ret=I(2^32); PUSHREG; NOT` | `[I(0)]`. |
| `ret=I(2^32); PUSHIS 0; PUSHREG; OR` | `[I(0)]`, because binary integer truth narrows. |
| `ret=I(2^32+1); PUSHF 0; PUSHREG; ADD` | `[F(4294967296)]`; full64 conversion, unlike integer ADD. |
| `ret=I(16777217); PUSHF 16777216; PUSHREG; EQ` | `[I(1)]` after f32 promotion. |
| `ret=I(2^32); PUSHIS 0; PUSHREG; EQ` | `[I(1)]`, because both are narrowed to s32. |
| `PUSHF 0; PUSHF 1; DIV` | `[F(+infinity)]` under nontrapping controls. |
| `PUSHF 0; PUSHF 0; DIV` | `[F(NaN)]`; no particular NaN payload promised. |
| `PUSHF 0.5; NOT` | `[I(0)]`, no integer truncation. |
| `PUSHF -0; NOT` | `[I(1)]`. |
| `PUSHIS 0; PUSHIS 2; OR` / `AND` | `[I(1)]` / `[I(0)]`, not bitwise result 2. |
| Push b=2 then a=1; `S`, `L`, `SE`, `LE`, `EQ`, `NEQ` separately | I(1), I(0), I(1), I(0), I(0), I(1), respectively. |
| Push b=F(0), a=F(qNaN); each comparator separately | EQ=0, NEQ=1, S=L=SE=LE=0; NOT(qNaN)=0. |

For a concrete remainder expansion with preseeded LI0=a, LI1=b, execute `PUSHLIX 1; PUSHLIX 0; DIV; PUSHLIX 1; MUL; PUSHLIX 0; SUB`. Expected single I results: (a,b)=(7,3)→1, (-7,3)→-1, (7,-3)→1, (INT32_MIN,-1)→0, (INT64_MIN,-1)→0, (7,0)→7. The fourth/sixth cases fault at DIV on Steam. This is a seven-instruction expansion, not an unsupported MOD opcode.

### Conversion vectors

Each row executes `PUSHF input; POPLIX 0; PUSHLIX 0`:

| f32 input | Switch final I value | Steam final I value, masked exceptions |
| --- | ---: | ---: |
| -3.75 | -3 | -3 |
| qNaN | 0 | INT64_MIN |
| +infinity | INT64_MAX | INT64_MIN |
| -infinity | INT64_MIN | INT64_MIN |
| +2^63 (`0x5F000000`) | INT64_MAX | INT64_MIN |
| Largest f32 below +2^63 (`0x5EFFFFFF`) | 9223371487098961920 | Same |
| -2^63 (`0xDF000000`) | INT64_MIN | INT64_MIN |

Synthetic tag-2 cell reference to GI0=2^32+1: integer argument gives 4294967297; float **argument** gives F(1); float **pop** gives F(4294967296). `MINUS` on that reference leaves its index/tag/depth unchanged and changes GI0 to -4294967297. `NOT` instead pops the reference and pushes I(0), leaving GI0 unchanged. These vectors require seeded carrier tags; ordinary PUSHIX would make a snapshot tagged 0 instead.

### Control, lifetime and capacity vectors

| Initial state / sequence | Expected observation |
| --- | --- |
| `PUSHF 0.5; IF L` | Consumed condition, PC falls through. F(-0) takes L; F(qNaN) falls through. |
| `ret=I(2^32); PUSHREG; IF L` | Consumed condition, PC becomes L, although NOT of the same value is 0. |
| Empty stack, CALL at PC 10 to procedure PC 40 whose body is `PROC; END` | CALL leaves `[A(10)]`, PC40; PROC→PC41; END pops A and resumes PC11, result unchanged. |
| Empty stack; `END` | Stop, depth0, result unchanged. |
| Procedure P and label L have different PCs; `JUMP P` / `GOTO L` | Respective table PC; neither pushes an address. |
| `RUN` / `PROC` | On pinned Switch, each advances one word, no stack or result change. |
| Invalid seeded A(10), execute IF L | Slot retained, PC=L. Seeded S(B) behaves the same. |
| Invalid seeded F(2.75), execute END | Pop it, PC=3 after diagnostic; not a float BF result. Seeded S(B) instead gives PC1. |
| ret=I(9), stack `[I(2), I(3)]`, COMM descriptor count2, native flag0 | Yield: arguments/depth/PC unchanged unless native explicitly changed them; ret stays9 if handler did not write it. |
| Same, native flag1 and no handler PC change/result write | Stack `[]`, PC advances once, ret stays9; subsequent PUSHREG produces I(9). |
| Same successful command explicitly sets PC100 and ret=I(7) | Depth drops by2, PC remains100 (not101), subsequent PUSHREG produces7. |
| New context, `PUSHREG` | `[I(0)]`. |
| ret payload preseeded `0x1234567800000000`, float setter receives1.0, `PUSHREG` | Tag1, raw payload `0x123456783F800000`; numeric float is1.0. |
| 46 ordinary entries; `PUSHIS 7` | Depth47, slot46=I(7), reserved result unchanged. |
| 47 ordinary entries; `PUSHIS 7` | Diagnostic returns; slot47/result becomesI(7), depth48. Not a supported extra ordinary entry. |
| Nonempty seeded S(B); `PUSHIS 2; ADD` | Binary float path consumes both, contributes +0 for S, produces F(2); not string addition or pointer arithmetic. |

## 7. PS2 contrast, corroboration limits and receipt

The PS2 reference is `decomp/src/Script/scrTraceCode.c`: `scrOpPushImmInt` at `0x0029A050`, `scrBinaryOp` at `0x0029B1B0`, `CodeFunc_Neg` at `0x0029BFB0`, `CodeFunc_JumpIfFalse` at `0x0029C750`, command accessors at `0x0029CC00`/`0x0029CD50`, and return setters at `0x0029CF50`/`0x0029CF70`. It uses s32/f32 payloads and four-byte integer banks. Golden widens carriers, integer banks, integer argument/result access and unary negate; **it retains the s32 binary temporaries and integer IF condition**. Replacing the entire PS2 integer lane with s64 arithmetic is therefore not faithful Golden behavior.

Steam's dispatch table at RVA `0xB25710`, reached from `0x46AB52`–`0x46AB68`, has the same 35 opcode correspondences. Ordinary evidence corroborates wide integer reads/stores, f32 storage, s32 binary SUB/MUL/DIV/comparisons, full64 unary negate/NOT, signed bank indices, slot47 result storage, COMM cleanup and the interpreter loop. Some Steam opcode bodies or shared push helpers are protected entry jumps: integer push `0x469240`→`0x289A8970`, PUSHIS `0x469560`→`0x289D0E60`, ADD `0x469FD0`→`0x28C6C2C0`, END `0x469A90`→`0x28C002D0`, IF `0x46AA90`→`0x28D2DDD0`, with other protected entries also left unopened. Their `.arch` destinations were not decoded. Thus **Switch-only details are not falsely labelled complete Steam equivalence**, especially malformed tag/index/stack continuations. The explicitly ordinary x86 divide/conversion differences must be retained in any Steam-target profile.

Raw research remains outside the repository in `<scratch>/GoldenRE-notes/switch/`: `vm_inventory.json`, `vm_tables.json`, `vm_cfg_receipt.json`, `vm_llvm_receipt.txt`, `vm_vectors_receipt.json`, `vm_steam_table.txt`, `vm_steam_inventory.txt`, per-function `vm_*.txt` evidence and analysis helpers. No decompiler output, instructions or retail data are committed here.

Static receipt commands used the fixed Windows Python312 under the specified Git Bash, not WSL:

- `python vm_inventory.py`: 35 opcode entries identified through exact NSO relative relocations.
- `python vm_tables.py`: all 15 internal byte-indexed dispatch tables resolved, including both twelve-way binary-operation tables and the type switches.
- `python vm_decompile.py <addresses>`: fixed RomWright analyse-function then Windows worker route; all requested VM/diagnostic entries returned successfully. Instruction evidence, not decompiler signedness alone, decides widths/NaN cases.
- `python vm_steam_inventory.py`, `python vm_steam_table.py`, `python vm_steam.py <rva:size>...`: executable reads guarded to `.shared`; exception-directory data supplied available function boundaries. The absent named `.pdata` section was handled through the PE exception-directory RVA instead. No protected destination was followed.
- `python vm_receipt.py`: **35 opcodes, 15 resolved tables, 40 CFG entry traversals, zero unresolved boundaries**; pivotal instructions independently decoded with `llvm-mc -triple=aarch64 -disassemble`.
- `python vm_vectors.py`: analytical wrap/divide/rounding/conversion/remainder values recorded; not an implementation or retail-runtime test.

No build, unit test, objdiff/verify, CI, game or emulator was run. The implementer's next check is to exercise these vectors against its real Golden-mode VM, preserving the documented Switch/Steam profile differences rather than reporting these static deductions as runtime passes.
