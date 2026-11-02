#ifndef P4PC_GAME_KNOWLEDGE_NATIVE_H
#define P4PC_GAME_KNOWLEDGE_NATIVE_H
#include <stdint.h>
#include "p4pc/game_elements_native.h"

/* Expanded-route learning at 001F14F0's eligible-outcome gate (enemy defender,
 * outcome not a miss), called only while p4_game_elements_native_enabled(),
 * once for each frame whose affinity was actually resolved against this
 * defender in that target row. defender is the same DatUnit given to the
 * native affinity query. It follows that resolver's route:
 *   - no element or a leaf without affinity: nothing is learned;
 *   - an authored enemy alias and side pair: that knowledge key is learned;
 *   - an unmodified retail component without one (the original resolver's
 *     route): the original retail bit for leaves below 8 (enemies below 336).
 * The caller never writes the retail bitmap itself on this route. Nonzero is a
 * P4_TABLE_* refusal (e.g. NOT_FOUND for an unrepresentable pair): the caller
 * refuses through its controlled runtime failure; nothing is guessed known. */
int p4_game_knowledge_native_observe(void *defender, const P4GameActionFrame *frame);
/* Expanded-route AI reads along the same route. ai_query makes a known
 * affinity ACTUAL and an unknown enemy affinity PROVIDED neutral 100, so a
 * hidden row never ranks a candidate. Persona defenders and frames without
 * affinity are known; an original-route component reads exactly the retail
 * datTestBitmapBit(enemy, leaf). any is 001DB740's Analyze any-known for an
 * enemy: its alias's side keys plus the original bits of natural retail pairs
 * the original resolver still answers. Errors leave outputs unchanged. */
int p4_game_knowledge_native_ai_query(void *defender, const P4GameActionFrame *frame,
    P4GameAffinityQuery *query);
int p4_game_knowledge_native_any(void *defender, uint32_t *known);
/* Analyze (00219790) per displayed pair, which resolves no action: the same
 * route with no owned binding. No affinity is known; a persona is INVALID; an
 * original-route pair reads its retail bit for leaves below 8 (else
 * NOT_FOUND). After the pair resolves, 001F0950's encounter override applies:
 * without enemy metadata flags 0x4004 and with story flag 0x1403, flag 0x1408
 * reveals and 0x1409 hides. AI reads never take this UI override. */
int p4_game_knowledge_native_analyze(void *defender, const P4GameActionElement *element, uint32_t *known);
#endif
