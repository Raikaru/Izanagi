# Publish a mod

[Modding home](../README.md) · [Troubleshooting](README.md)

1. Give your mod a unique lowercase id, a display name and a version. Keep all
   six required manifest fields, and list required features and revisions with
   `requires`; see the [capability reference](../capabilities.md) and
   [API index](../reference/README.md).
2. Include your own `mod.ini`, `main.luau` and authored files in a single mod
   directory. Document what it changes, any settings and required port version.
   Do not include an ISO, extracted retail data, game assets, executable or saves.
3. Try a fresh installation and a fresh save copy, enable the package and
   restart, then exercise its actual features and any toggle in game. The
   optional checker catches some script errors, but a clean checker result is
   not gameplay evidence. Explain any known compatibility limits, including
   load order and raw-address dependencies.
4. Share the directory as a zip; players extract the directory into their mods
   root and enable it in MODS. Changing selection requires restart. Ask users
   for logs and reproduction steps, never for their game files or saves.

There is currently no released `p4mod pack` command. Do not advertise planned
tools as available. See the [manifest reference](../files.md#package) for id, ordering and file rules.
