# Native battle notifications (One More, battle result rewards and level-ups): core, because the mirror
# calls them in every build (config/mirror_fixes.txt); the semantic events install the sink.
target_sources(p4port PRIVATE src/platform/mods/battle_notify.c)
target_sources(p4port PRIVATE src/platform/mods/battle_action_exec.c
  src/platform/mods/battle_action_exec_native.c)
