# Golden mode

Golden mode is an in-development alternative to the existing PS2 mode. It is intended to derive its content at runtime from the player's own Steam installation; no game content is distributed with the port. The mode switch and ID mapping described here do not yet make Golden mode playable.

`[game] mode = ps2` is the default; `[game] mode = golden` selects Golden mode. `P4PC_MODE` overrides the setting. The mode is resolved once and remains fixed for the process. An unknown value is invalid.

Golden startup fails closed if native package preparation fails or the table boot has not published all nine inputs from the same verified Golden cache. It never offers the PS2 optional-package fallback in that mode. PS2 retains its existing recovery behavior.

## ID domains

The map preserves PS2 IDs and places Golden-only entities in the existing expansion families. For the measured Golden table shape, let `A = active_skills - 440` (32).

| Domain | Golden ID | Port ID |
| --- | --- | --- |
| Skill | below 440 | unchanged |
| Skill | 440 through `active_skills - 1` | `576 + id - 440` |
| Skill passive | `id - A < 576` | `id - A` |
| Skill passive | `id - A >= 576` | unchanged |
| Enemy, item, persona | any | unchanged |

IDs at or above the retail enemy, item and persona counts (336, 1536 and 256 respectively) are declared as expanded entities. The measured shape has 48 expanded skills, 32 enemies, 1024 items and no personas. Declarations are permitted only in Golden mode and must cover every expanded ID exactly once before registration begins.

Golden-only skill effects and enemy or persona models need presentation sources. Native Golden asset support is planned after the table milestones; an entity without an acceptable source must not silently use an unrelated effect or model. Converters must map IDs inside table rows as well as row indices.

The Golden declaration assembler consumes already-converted rows, an explicitly approved retail model source for every expanded enemy or persona, and an approved icon reference for every expanded item. It does not infer an asset from a model-table row or invent an icon. Golden-only skills carry no effect source, so casting them is refused until an approved presentation source exists.

The in-development readers validate PAK v2, FTD and DAT framing without treating padding as payload. The EXP reader accepts only the measured Steam executable and EXP table hashes, locates the table structurally in its file-backed `.sdata`, requires that section to be initialized non-executable data, and never executes or unpacks the executable. It hashes the entire executable for build identity. Golden boot admits only registered compiled adapters and reports each missing required capability before reading or publishing tables.

The measured Steam install cannot boot in Golden mode at this stage. Required adapters remain unavailable, including EFFECT0 (472×12 versus 440×4), MODEL0 (11×382 versus 11×332), AICALC9/10 compatibility, approved expanded asset sources and text bindings. Twelve-material fusion recipes and 28-arcana chart consumers require M5b; no data is silently truncated.

The approved-output assembler prepares the retail prefixes of ITEM, SKILL and UNIT, the converted PERSONA and ENCOUNT banks, and the declaration metadata, affinity, AI-row, skill-auxiliary and inheritance-rate inputs. It leaves unresolved cache entries absent for the responsible approved adapters to fill; it does not synthesize EFFECT, MODEL, AI programs, icons or text.

The private table cache lives under the user's SDL preference directory in `golden-cache`, outside the game install, program directory and source checkout. Its key covers the source hashes and converter, specification and schema versions. A complete entry contains a manifest, a verified table index and SHA-256-addressed objects; damaged entries are treated as misses for rebuilding rather than trusted. Cache payloads are user-derived and must not be added to source control or distributed.

The cache refuses unsafe paths and old keyed entries containing unexpected files it cannot prune; it does not recursively delete unknown content.

Synthetic cache tests also keep their valid preference fixtures outside the checkout:
the shared `golden_cache_fixture.h` helper selects Windows `TEMP` or POSIX `TMPDIR`
(default `/tmp`). Missing or empty Windows `TEMP` fails the test, with no checkout
fallback. A build directory below a checkout is not a valid preference root.
The cache test separately verifies that an unreported checkout ancestor is
rejected; this safety rule applies on both 32-bit and 64-bit Windows as well as POSIX.

The source identity collector records the verified executable, English battle and extra tables, fusion members, inheritance rates and rank-up member under their full inventory chains. It includes the pinned Steam build and the converter, specification and schema versions. The identity remains valid only for the exact checked inputs; it does not itself publish the tables.

The inheritance-rate reader verifies the measured English archive member hash. Its converter uses the skill ID map for every rate, including Golden's shifted passive skills, and requires a one-byte rate for each skill in the supplied domain.

The Social Link rank-up reader verifies the measured English archive member hash. Its converter maps both the taught and replacement skill IDs and refuses malformed table shapes and out-of-domain skills.

The skill table projection maps element rows and projects 44-byte Golden active metadata to 40-byte PS2-shaped rows. Golden's widened flag bytes and presentation data need separate support; the projection does not make those Golden-only effects available.

The whole-bank projection assembles the existing skill, enemy, item, name, persona and encounter converters without using PS2 table bytes. It retains expanded-ID rows in the projected banks for later declaration and refuses an over-width name bank rather than returning a partial projection.

The ITEMTBL converter checks both section headers, name encoding and every granted-skill ID before writing the PS2-shaped metadata and names. Empty name slots remain empty and cannot be used as declaration names.

Battle-name conversion refuses names that do not fit the PS2 field; it never silently truncates them. On the measured English Steam build, skill and persona name banks contain overlong names, so they require widened name readers before those banks can be used.

Encounter projection maps formation enemies and record items, and repacks weighted-group rate headers. The consumer returns no encounter when a selected category has zero eligible weight; the converter preserves those valid empty categories.

Persona growth, party learn-event and skill-replacement projections map active skill references while preserving non-skill event payloads and the unused tail after a terminator.

Enemy metadata projection maps learned skills and item drops separately; the flagged special drop is an item ID, despite misleading legacy skill-oriented field names.

The fusion source reader pins the measured English members. Projection checks tier, affinity, gate and chart shapes, maps gate Persona IDs and bounds chart arcana indices. Native use of the wider charts requires corresponding consumer support.

Additional pinned English inputs cover EFFECT, MODEL, AICALC and the camp table pack. Their presence does not establish compatibility of opaque Golden AI programs or provide native Golden asset support.

Twelve-material fusion recipes cannot fit the PS2 eight-material representation. M5 must explicitly refuse and log those recipes; a separate Golden-only widening milestone is needed before they can be used. PS2 fusion and save layouts remain unchanged.
