# RE-3 P10: encounter zero-rate assertion audit

Revision 2026-11-12; companion to [item categories](item-categories.md).
This separates **category rate total zero** from **selected category's
eligible formation-weight sum zero**. They have different control flow.
No game execution, payload, instruction listing or retail capture is included.

## Sources and evidence classes

- **OBSERVED-S/data:** English Steam build 10836033
  `data_e.cpk::init_free.bin::battle/ENCOUNT.TBL`, SHA-256
  `6391a03228554a7d40cba108a7ba24cf813d2a719e0bba2f2ef06dc6c726910e`.
  Block 3 contains 68 × 252-byte weighted groups. Block 1 contains 180
  selectors; block 0 contains 944 formations. Original framing is in
  [battle_tables.md](../battle_tables.md).
- **OBSERVED-P/code:** PS2 `src/Kosaka/k_encount.c`,
  `Encount_PickEncounter`, `0x00161630`: category-total assertion at source
  lines 207–210, empty-category return at lines 229–242. The projected row
  has stride 248 and the first two rate columns, not the third Golden byte.
- **OBSERVED-N/code:** Switch `main.nso`, SHA-256
  `854c7a1e33de1619abf009fa7f32b8c012960d685c8bf1bf99a92a6a4d0bda1c`,
  runtime base `0x7100000000`, patch 131072. Weighted selector
  `0x71001BC650`–`0x71001BCA6C`; selector table pointer comes through
  GOT `0x7100880AB0`, weighted table pointer through `0x7100880AB8`.
  Category-total assertion `0x71001BC804`–`0x71001BC810` cites
  `field/k_encount.c`, line 0x6F. Empty eligible-weight return is in
  `0x71001BC928`–`0x71001BC9C8`.
- **OBSERVED-N/instruction corroboration:** independently decoded LLVM
  ranges `0x71001BC6C0`–`0x71001BC788` and
  `0x71001BC9F8`–`0x71001BCB10` establish signed byte rate loads, flag
  arguments, mode branches, integer multiplier and assertion predecessor.
  Raw notes stay private. The decompiler's indirect-control limitations are
  not used as evidence that a branch is absent.
- **OBSERVED-S/code:** ordinary `.shared` selector `0x2C9D00`–`0x2CA197`.
  Its complete total calculation is `0x2C9E08`–`0x2C9EB9`: signed-byte
  rates, stride 252, forced modes 3/4 with normal-rate fallback, and ordinary
  mode 0/1/2+ additions agree with the Switch formula below. It tests bits
  16/17 of the flags word at bitmap +0x280 (flag IDs 0x1410/0x1411).
  Normal/boost floating constants loaded at `0x2C9D35`/`0x2C9E1B` are
  exactly 1/100, at `.sdata` RVAs `0x964D0C`/`0x965350`; conversion to
  integer before rate multiplication is exact. No PS2-style field
  suppression enters this total. This ordinary range has no zero-total
  assertion between total construction and category drawing; the assertion
  being audited belongs to the PS2 consumer (and is also present on Switch).
  Steam returns -1 for zero eligible weight at `0x2CA118`–`0x2CA179`.

**Status:** pinned Steam data and the ordinary Steam, Switch and PS2 total
calculations admit a complete finite proof of nonzero category total.
No `.arch` target was entered. This is static arithmetic/control-flow
evidence, not retail execution or a complete encounter liveness proof.

## Rate header and runtime variables

**OBSERVED-N:** for category c = normal/special/rare (0/1/2), the first
and second rate bytes are at `3*c + d`, with `d` constrained to 0 or 1.
The consumer uses **signed** byte loads for these rates. Weather query
`0x71001D10D0` selects the alternate column for weather values 1,3,7;
all other values select the first column. The repeated weather reads in the
selector are not calls to calendar `calGetDayEntryByPhase` as on PS2.

**OBSERVED-N, `0x71001D10D0`:** when its stored date/time equals the
current date/time, weather overrides are tested in priority order:
flags 0x1203,0x1204,0x1205,0x1206,0x1207,0x1208 yield values
0,1,2,3,4,7 respectively; first set flag wins. Otherwise, or with no
override set, it reads the calendar/weather table. All 64 Boolean override
combinations and both date/time-match outcomes still select only d=0/1.
The matrix therefore covers them by both columns, without assuming the
six override flags are mutually exclusive. Steam's selector similarly
collapses its weather getter's byte result to the same two columns
(`0x2C9D99`–`0x2C9E08`), regardless of the getter's internal flag state.

**OBSERVED-N:** Golden header bytes +5 and +8 also enter special/rare
retry thresholds when the fifth argument equals 1: the selector calls
`0x71001F2290(2)` / `(3)` and retries if the signed header threshold is
≤ the helper's signed-byte result. Consequently, third bytes are **not a
third selectable day column** in this consumer. The normal triple's third
byte is not used to compute this total. Do not feed byte +2 (including its
0xFF sentinel) into a third-day signed rate calculation.

**OBSERVED-N:** let N,S,R denote the three signed rates of selected column
`d`, and let:

- `F = flag(0x1410)`; nonzero excludes special and rare additions on the
  ordinary mode path;
- `B = flag(0x1411)`; multiplier M is 100 if its return is exactly 1,
  otherwise 1;
- `mode = low16(fourth argument)`;
- `retry = signed16(fifth argument) == 1`.

`retry` affects selection retries **after** the assertion, not the computed
total. Out-of-band field/room returns -1 before reading a weighted row.
The ordinary total calculation is at `0x71001BC9F8`–`0x71001BCA50`;
forced modes 3/4 are at `0x71001BC730`–`0x71001BC784`.

| Mode equivalence class | Steam / Switch category total T | Reads F? |
| --- | --- | --- |
| 0 | N | yes, but no addition |
| 1 | N + (F == 0 ? M*S : 0) | yes |
| 2 | N + (F == 0 ? M*S + R : 0) | yes |
| 3 | M*S if nonzero, otherwise N | no |
| 4 | R if nonzero, otherwise N | no |
| every u16 value 5–65535 | N + (F == 0 ? M*S + R : 0) | yes |

These six classes cover the **entire** u16 argument domain, not just mode
values seen at one caller. They do not assert that every value is normally
reachable. The eight explicitly identified Switch flags are 0x1410/0x1411
and the six weather overrides; all weather/other getter state is collapsed
to both possible columns. Retry-helper internals are a separate unknown.

## PS2 consumer with the Golden first-two-column projection

**OBSERVED-P:** let `L = low16(level argument)` with three equivalence
classes 0,1,≥2. Let `A` be the PS2 calendar-derived alternate-day bit.
The rates are the Golden row's first two columns repacked into the PS2
header. Let `E` mean field ID 0x28 or 0x3C and `flag(0xC0F) == 0`.
The PS2 multiplier is 0 if E, otherwise 100 when flag 0x1411 ==1,
otherwise 1. The 0xC0F condition **overrides** the boost condition.

Total is N plus M*S when L>0 and flag 0x1410==0, plus R when L≥2 and
flag 0x1410==0. The assertion fires only when that sum equals zero.
PS2 does not implement Switch's forced modes 3/4 or its threshold-retry
parameter; do not silently reinterpret the existing PS2 level argument as
the new Golden mode enum.

**OBSERVED-N:** the cited Switch selector has no 0xC0F/field-0x28-or-0x3C
multiplier-zero branch. That PS2 suppression is specified here for the
existing projected consumer, not asserted to be Golden balance. Whether a
Golden adapter should retain or replace that branch depends on corroborated
Golden routing, not on the nonzero-total proof.

## Full pinned-data result: all 68 groups

**OBSERVED-S/data:** for all 68 rows and both columns:

- normal rate is strictly positive (measured range 55–110);
- special and rare rates are nonnegative signed bytes;
- no selected-column rate is a negative signed byte;
- all positive-rate/empty-eligible-weight observations are normal category,
  not all-category rate zeros.

**STATIC DERIVATION from those observations and the code above:**

- Every ordinary Switch or PS2 total is at least N >0, since each added
  term is nonnegative, even when PS2 multiplier is zero or F excludes both
  additional categories.
- Switch mode 3 has a positive M*S or falls back to positive N. Mode 4 has
  positive R or falls back to positive N.
- Maximum possible arithmetic with signed-byte nonnegative rates and
  multiplier 100 is far below signed32 overflow. No wrap-to-zero case is
  being hidden by an assumed unlimited integer.

**OBSERVED offline arithmetic census:** 3,264 Switch matrix cells =
68 groups × 2 columns × 2 F states × 2 B states × 6 mode classes:
**zero zero-total cells**, minimum total 5. Fifth-argument retry false/true
are both covered by total independence, giving 6,528 equivalent cells if
that dimension is expanded. No RNG result influences this predecessor.

**OBSERVED offline arithmetic census:** 3,264 projected-PS2 matrix cells =
68 × 2 columns × 2 F states × 2 B states × 2 E states × 3 L classes:
**zero zero-total cells**. E represents both 0xC0F states in the two special
fields plus every ordinary field (where either flag state maps to E=false).
Thus field suppression, boost, normal-only and level gating have all been
accounted for, not merely the all-flags-clear case.

### Every-group coverage ledger

Each group below has both rate columns checked against the two complete
matrices. “Empty” means positive category rate with no eligible positive
formation weight, **not** T=0. Group IDs are zero-based.

| Group IDs (inclusive) | Count | Category-total assertion result, all matrix states | Empty positive-rate categories |
| --- | ---: | --- | --- |
| 0 | 1 | unreachable by the audited formulas | normal, both columns |
| 1–7 | 7 | unreachable by the audited formulas | none |
| 8–15 | 8 | unreachable by the audited formulas | none |
| 16–23 | 8 | unreachable by the audited formulas | none |
| 24–31 | 8 | unreachable by the audited formulas | none |
| 32–39 | 8 | unreachable by the audited formulas | none |
| 40–47 | 8 | unreachable by the audited formulas | none |
| 48–55 | 8 | unreachable by the audited formulas | none |
| 56–63 | 8 | unreachable by the audited formulas | none |
| 64–67 | 4 | unreachable by the audited formulas | normal, both columns |

Counts sum to **68**. The five groups in M5 are exactly **0,64,65,66,67**;
there are ten empty category/column combinations. A previous “5 of 68”
observation was not evidence that the other 63 groups or flag combinations
were unauditable. All five retain positive normal rates, so suppressing
special/rare cannot turn their category total into zero.

**OBSERVED-P/N:** after choosing category, both audited consumers sum
weights only for nonzero formation IDs over ranges [0,20), [20,25),
[25,30), respectively, in the selected 30-entry list. A zero eligible sum
returns -1 **before** the second weighted RNG modulo. This is the safe
empty-selected-category case accepted by M5, not a reason to reject the
entire table during conversion. The Switch return is 0xFFFFFFFF; successful
results carry category bits 0x10000/0x20000/0x40000 plus formation ID.

## Important non-conclusions and BBV

### E1: Runtime receipt and observable encounter balance (not exercised)

**NOT EXERCISED:** the ordinary Steam total predecessor is corroborated
above; its equivalence is no longer an unknown requiring a guessed helper.
This lane did not run the port or the game. The integrator still needs the
affected native-consumer matrix tests, not just this arithmetic census.

Proposed optional owner BBV: with repeatable dungeon saves, record ordinary
versus alternate weather and normally reachable normal-only/special/rare
conditions and boost effects. Freeze categories, no-encounter returns and
hangs/crashes; use N≥30 strata and a preregistered distribution test at
α=0.05. Unreachable debug flags and unavailable groups remain statically
covered, not falsely labeled owner-exercised. This checks observable
balance; it is not needed to replace the now-observed total formula.

### E2: Retry liveness and threshold-helper meaning (unresolved)

**OBSERVED-N, `0x71001BC7A8`–`0x71001BC7FC`,
`0x71001BC840`–`0x71001BC91C`:** positive total does not guarantee loop
termination. Forced mode 3 with S=0 falls back to N for its total but its
selection loop continues requiring a draw below M*S=0, which cannot
succeed for a nonnegative draw. Retry=true also rejects special/rare
choices according to +5/+8 and `0x71001F2290`. The helper's state domain and
all callers' reachability are not proven here. This is **not** an
all-zero-total assertion and must not be “fixed” by picking a random normal
formation or silently dropping retry conditions.

Proposed BBV: identify a normally reachable special/rare forced caller from
scripts, then capture accept/cancel or encounter completion on a save with
known accessible category stock. Compare N≥30 trials; explicitly record
nontermination. Do not attempt an unreachable malformed forced mode in the
owner's game. Supplement with static caller/threshold-writer audit and
synthetic bounded-loop diagnostics on the port; no timeout observed in this
lane, and no claim that the retail game reaches the zero-special forced
mode. E2 is tracked separately from P10's assertion arithmetic.

## Proposed implementation fixtures (not executed here)

1. Exhaust all 68 **locally read** rows through the real Golden/projected
   consumer's total-calculation seam for the matrices above. Record counts
   and hashes only; keep game data out of source control. Check the real
   signedness, flag ordering and suppression override, not a mock returning
   a precomputed pass result.
2. Synthetic ordinary row N=40,S=0,R=10: every F/B/L/suppression state has
   nonzero total. Synthetic forced mode 3 uses N when S=0 for the assertion
   calculation, but does **not** thereby acquire a valid selection outcome.
3. Synthetic N=0,S=0,R=0 must reach the structural assertion/refusal before
   RNG division. Do not weaken the assert, invent a default rate, or treat
   this invalid synthetic input as one of the five valid Steam groups.
4. Synthetic normal-rate-positive row with all eligible formation weights
   zero returns -1 before the weighted modulo. Distinguish formation ID=0
   entries from valid positive-ID entries; test both columns and all three
   category slices.
5. Keep PS2 and Golden mode enums separate. PS2 multiplier-zero behavior
   and Switch forced-mode/retry behavior need independent routed fixtures;
   identical table shapes do not establish identical selection algorithms.

**Admission statement:** 68/68 groups and every total-affecting flag class
of the audited ordinary Steam, PS2 and Switch consumers are statically
covered. P10's zero-total arithmetic question is resolved for the pinned
data. E2 and native M7d qualification are separate; no runtime receipt or
permission to remove a boot refusal is manufactured by this document.

Import note: re-imported by the Golden M7d step from the committed RE-3 specification; the M6c intake step had left it out of the specification tree.
