# Mod banners and message lines in battle (p4.battle.announce/say): core, because the mirror's
# btlActionStartInit/EndInit (config/mirror_fixes.txt) call it in every build; the Luau binding
# lives in p4_mod_game.
target_sources(p4port PRIVATE src/platform/mods/battle_present.c)
