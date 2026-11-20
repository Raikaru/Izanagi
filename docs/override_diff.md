# Override differential testing

`tools/mipsdiff/ovdiff` compares a compiled native implementation with the
corresponding retail instructions interpreted on the same guest memory. It
uses POSIX process and signal facilities and is not built on Windows or macOS.
It requires the executable from your own USA disc; no game data is included in the
repository. Keep the executable and any resulting captures outside the checkout.

The harness generates deterministic test cases, runs each side from the same
initial state and compares results and guest-memory writes. It can use native
implementations for callees or interpret them as well. Invalid inputs,
unsupported instructions and missing callees are not evidence of equivalence.
Floating-point differences require special care: a tolerance match alone is not
proof that the underlying arithmetic is identical.

After building the tool, its command line accepts a retail executable and a
function address, a list of addresses or `all`. For example:

```sh
ovdiff -n 40 -o out.tsv /path/to/SLUS_217.82 all
```

Use `-c N -v` to replay a case with diagnostics. `P4_TRACE=1` enables the
interpreter's instruction trace. The `ovdiff_subset` CTest uses the checked-in
subset when its game input is available; without the required game data it skips.
Consult `tools/mipsdiff/` for signatures, supported instructions and harness
options. The comparison complements, but does not replace, integration tests or
real gameplay.

## VU0 sums

VU0 arithmetic differs from ordinary host floating-point arithmetic. Preserve the
original operation order and use the port's VU0 helpers where required; see
[porting](porting.md) for the guest arithmetic model. A passing generated case
covers only the inputs and effects actually compared.
