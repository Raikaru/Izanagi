/* The C declarations of session.zig (its `c_bindings` module): cmake/zig/zig_object.py runs `zig translate-c` on this header
   with the object's own target, libc, definitions and include directories. Angle includes, as @cInclude was. */
#define _POSIX_C_SOURCE 200809L
#include <p4pc/battle_sim.h>
#include <p4pc/battle_live.h>
#include <p4pc/gmem.h>
#include <p4pc/mod_fidelity.h>
#include <p4pc/battle_events.h>
#include <p4pc/battle_prepare.h>
#include <p4pc/content_digest.h>
#include <p4pc/game_items.h>
#include <p4pc/game_items_native.h>
#include <p4pc/game_table_extents.h>
#include <p4pc/mods.h>
#include <p4pc/walk_script.h>
#include <memcard/mod_save_transport.h>
#include <pad/pad_sdl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
