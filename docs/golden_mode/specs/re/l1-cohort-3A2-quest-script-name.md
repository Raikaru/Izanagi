# L1 cohort 3A2: quest script task name

**1 PROVEN / 32 PS2 bytes:** `0064b320`, `camp_quest_script`.

`l1-cohort-3A2-quest-script-name-proof.json` seals the actual 18-byte Steam `.xpdata` name at RVA `b07550` and the ordinary `.shared` wrapper/factory/callback witnesses. This is positive named-lookup plus actual registration/lifecycle correspondence, not text equality or a module label.

Old script wrappers `354500` / `354660` pair with Steam `17b6b0` / `17b710` and NSO `f6ff0` / `f7350`. Both look up the same named task. When absent they read VM parameter 0, subtract one and invoke the common factory with mode 0 or 1; while pending they return zero. Completion destroys the task and returns one. **Golden tests terminal state 5 at native work+8, not PS2 state 3.**

Steam `17b360` and NSO `f7050` positively register that same name at priority 15 with update/destroy callbacks and `0x1c` native work, versus PS2 `0x10`. The shared native setup/quest/window state replaces the old notice internals; merely renaming the old four-state task is not a valid binding. Steam callback witnesses are `17ae90` / `17b160`; NSO factory names the corresponding `fc960` / `fcba0` callbacks.

Required seam: **`golden-native-driver:quest-script-notice`**, status **pending-seam**. Migrate both wrappers, lookup, native task lifecycle, VM arguments and actual notice/quest/window providers together. No guest work or callback copies, invented completion fallback, active PS2 name fallback, or runtime NSO/private-cache input. This source decision is not runtime acceptance.
