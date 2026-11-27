#ifndef P4PC_GOLDEN_LITERALS_H
#define P4PC_GOLDEN_LITERALS_H
#include <stdint.h>
/* These native readers require the sealed Golden image transaction. */
int32_t p4_golden_literal_key_action(uint32_t key);
uint32_t p4_golden_literal_message(unsigned month, int column);
float p4_golden_literal_scale(unsigned rank);
float p4_golden_literal_weather(unsigned rank);
/* Both PS2 date-table copies resolve through one sealed Golden descriptor. */
int p4_golden_literal_date_find(int month, int day);
int32_t p4_golden_literal_date_constraint(int index, unsigned field);
uint32_t p4_golden_literal_date_effect(int index, unsigned slot);
/* Golden-only companion state; the PS2 work block stays byte-for-byte intact. */
void p4_golden_literal_weather_work_register(uint32_t work);
int p4_golden_literal_weather_work_release(uint32_t work);
unsigned p4_golden_literal_weather_selected_count(uint32_t work);
uint32_t p4_golden_literal_weather_selected_effect(uint32_t work, unsigned slot);
void p4_golden_literal_weather_roll(uint32_t work, int index, uint32_t (*random)(void));
/* Ordinary Golden producer arithmetic; pure and allocation-free. */
int p4_golden_literal_roll_index(int level, uint32_t stage_random, uint32_t direction_random);
/* Complete code-built type-3 event-filter extra; no legacy template read. */
void p4_golden_literal_filter3_extra(uint8_t out[44], const uint8_t record[44]);
#endif
