#!/usr/bin/env python3
"""Translate the retail EE newlib allocator, preserving every branch/delay slot.

Pointers/chunk headers/bin links remain four-byte guest words on every host.
The only external operations are byte copies, modeled IRQ brackets and sbrk.
No retail instruction bytes or allocator source from another newlib are embedded.
"""
import argparse
from pathlib import Path
import re

import mips2c_double as scalar

FUNCTIONS = ((0x43c738, 0x43c7f0), (0x43e170, 0x43e450),
             (0x43e450, 0x43e5b0), (0x43ed28, 0x43ef80),
             (0x43ef80, 0x43f668), (0x43fa88, 0x43fae0),
             (0x43fae0, 0x43fb38), (0x441580, 0x441b10),
             (0x441ef0, 0x441f40))


def simple(m, operands):
    r = [scalar.operand(v) for v in operands]
    if m in ('daddiu', 'daddu', 'dsubu'):
        rhs = str(scalar.integer(operands[2])) if m == 'daddiu' else r[2]
        return f'{r[0]} = {r[1]} {"-" if m == "dsubu" else "+"} (uint64_t)({rhs});'
    if m == 'dnegu':
        return f'{r[0]} = UINT64_C(0) - {r[1]};'
    if m in ('dsllv', 'dsrlv', 'dsrav'):
        cast = '(int64_t)' if m == 'dsrav' else '(uint64_t)'
        return f'{r[0]} = {cast}{r[1]} {"<<" if m == "dsllv" else ">>"} ({r[2]} & 63);'
    if m == 'mult':
        if len(r) != 3:
            raise ValueError(f'unexpected multiply operands: {operands}')
        return f'{r[0]} = D_SEXT32((uint32_t){r[1]} * (uint32_t){r[2]});'
    if m in ('ld', 'lw', 'lwu', 'sd', 'sw'):
        offset, base = re.fullmatch(r'(-?\d+)\((\w+)\)', operands[1]).groups()
        offset = int(offset)
        if base == 'sp':
            return scalar.simple(m, operands).replace('dlibm_', 'heap_frame_')
        address = f'(uint32_t)({base} + (uint64_t)(int64_t){offset})'
        if m in ('sd', 'sw'):
            return f'heap_store{64 if m == "sd" else 32}({address}, ({"uint64_t" if m == "sd" else "uint32_t"}){r[0]});'
        value = f'heap_load{64 if m == "ld" else 32}({address})'
        return f'{r[0]} = {"D_SEXT32(" + value + ")" if m == "lw" else value};'
    return scalar.simple(m, operands)


def call(address):
    if address in {lo for lo, hi in FUNCTIONS}:
        return f'heap_{address:08x}(a0, a1, a2, a3)'
    if address == 0x43f810:
        return 'heap_copy((uint32_t)a0, (uint32_t)a1, (uint32_t)a2)'
    if address == 0x43f9c8:
        return 'heap_set((uint32_t)a0, (uint32_t)a1, (uint32_t)a2)'
    if address == 0x421ee0:
        return 'p4pc_ee_heap_sbrk((int32_t)a0)'
    if address == 0x4222b0:
        return 'QueryIntrContext()'
    if address == 0x42ba20:
        return 'D_SEXT32(func_0042ba20())'
    if address == 0x42ba70:
        return 'D_SEXT32(func_0042ba70())'
    raise ValueError(f'untranslated allocator callee {address:08x}')


def lift(lo, hi, instructions):
    used = {'v0'}
    for m, operands in instructions.values():
        used.update(v for v in operands if v in scalar.REGS and v not in ('a0', 'a1', 'a2', 'a3'))
        for operand in operands:
            match = re.fullmatch(r'-?\d+\((\w+)\)', operand)
            if match and match[1] not in ('sp', 'a0', 'a1', 'a2', 'a3'):
                used.add(match[1])
    out = [f'/* Retail {lo:08x}..{hi:08x}; every instruction and delay slot retained. */',
           f'static uint64_t heap_{lo:08x}(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)', '{']
    out.append('    uint64_t ' + ', '.join(r + (' = UINT64_C(0x7690f0)' if r == 'gp' else ' = 0') for r in scalar.REGS if r in used) + ';')
    if any('(sp)' in ','.join(operands) for m, operands in instructions.values()):
        out.append('    unsigned char frame[128];')
    for address, (m, operands) in instructions.items():
        out.append(f'L_{address:08x}:')
        if m in scalar.BRANCHES:
            target = int(operands[-1], 16)
            if not lo <= target < hi:
                raise ValueError(f'{address:08x}: out-of-function conditional branch')
            slot = simple(*instructions[address + 4])
            if m == 'b':
                out.append(f'    {slot} goto L_{target:08x};')
            else:
                if m.endswith('l'):
                    out.append(f'    if ({scalar.condition(m, operands)}) {{ {slot} goto L_{target:08x}; }}')
                else:
                    out.append(f'    {{ int taken = ({scalar.condition(m, operands)}); {slot} if (taken) goto L_{target:08x}; }}')
                out.append(f'    goto L_{address + 8:08x};')
        elif m in ('jal', 'j', 'jr'):
            out.append('    ' + simple(*instructions[address + 4]))
            if m == 'jr':
                if operands != ['ra']:
                    raise ValueError(f'{address:08x}: computed jump')
                out.append('    return v0;')
            else:
                target = int(operands[0], 16)
                if m == 'j' and lo <= target < hi:
                    out.append(f'    goto L_{target:08x};')
                elif m == 'j':
                    out.append(f'    return {call(target)};')
                else:
                    out.append(f'    v0 = {call(target)};')
                    out.append(f'    goto L_{address + 8:08x};')
        else:
            out.append('    ' + simple(m, operands))
    out.append('}')
    return '\n'.join(out)


PRELUDE = '''/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Generated by tools/mips2c_heap.py from the retail ELF disassembly.
 * Integer-only hand translation: original EE chunk sizes, addresses, bin links,
 * split/coalescing/reuse order and calloc/realloc/trim branches. */
#include <stdint.h>
#include <string.h>
#include "p4pc/gmem.h"
#include "ee_heap.h"
#include "p4pc/profile_trace.h"

extern uint32_t QueryIntrContext(void);
extern int32_t func_0042ba20(void);
extern int32_t func_0042ba70(void);
#define D_SEXT32(x) ((uint64_t)(int64_t)(int32_t)(uint32_t)(x))
static uint32_t heap_load32(uint32_t p) { uint32_t v; memcpy(&v, P4_G2H(p), 4); return v; }
static uint64_t heap_load64(uint32_t p) { uint64_t v; memcpy(&v, P4_G2H(p), 8); return v; }
static void heap_store32(uint32_t p, uint32_t v) { memcpy(P4_G2H(p), &v, 4); }
static void heap_store64(uint32_t p, uint64_t v) { memcpy(P4_G2H(p), &v, 8); }
static uint32_t heap_frame_load32(const void *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static void heap_frame_store32(void *p, uint32_t v) { memcpy(p, &v, 4); }
/* Realloc's predecessor merges copy toward a lower, overlapping address.
 * EE memcpy 0043f810 copies forward; memmove preserves those bytes without
 * invoking the host memcpy overlap UB. Fresh-block copies are disjoint. */
static uint64_t heap_copy(uint32_t d, uint32_t s, uint32_t n) { memmove(P4_G2H(d), P4_G2H(s), n); return d; }
static uint64_t heap_set(uint32_t d, uint32_t c, uint32_t n) { memset(P4_G2H(d), (unsigned char)c, n); return d; }
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', required=True, type=Path)
    parser.add_argument('--objdump', default='mipsel-linux-gnu-objdump')
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    all_instructions = [(lo, hi, scalar.instructions(args.objdump, args.elf, lo, hi)) for lo, hi in FUNCTIONS]
    out = [PRELUDE]
    for lo, hi in FUNCTIONS:
        out.append(f'static uint64_t heap_{lo:08x}(uint64_t, uint64_t, uint64_t, uint64_t);')
    for lo, hi, instructions in all_instructions:
        out.extend(['', lift(lo, hi, instructions)])
    out.extend(['', '''uint32_t p4pc_ee_heap_malloc(uint32_t reent, uint32_t size)
{
    uint64_t start = p4pc_prof_span_begin();
    uint32_t result = (uint32_t)heap_0043ef80(reent, size, 0, 0);
    p4pc_prof_span_end(start, "heap.malloc", size, result, reent, NULL);
    return result;
}
void p4pc_ee_heap_free(uint32_t reent, uint32_t pointer)
{
    uint64_t start = p4pc_prof_span_begin();
    (void)heap_0043e170(reent, pointer, 0, 0);
    p4pc_prof_span_end(start, "heap.free", 0, 0, pointer, NULL);
}
uint32_t p4pc_ee_heap_calloc(uint32_t reent, uint32_t count, uint32_t size)
{
    uint64_t start = p4pc_prof_span_begin();
    uint32_t result = (uint32_t)heap_0043c738(reent, count, size, 0);
    p4pc_prof_span_end(start, "heap.calloc", count, result, size, NULL);
    return result;
}
uint32_t p4pc_ee_heap_realloc(uint32_t reent, uint32_t pointer, uint32_t size)
{
    uint64_t start = p4pc_prof_span_begin();
    uint32_t result = (uint32_t)heap_00441580(reent, pointer, size, 0);
    p4pc_prof_span_end(start, "heap.realloc", size, result, pointer, NULL);
    return result;
}''', ''])
    args.out.write_text('\n'.join(out), encoding='utf-8')
    print(f'Generated {args.out}: {sum(len(code) for _, _, code in all_instructions)} original allocator instructions')


if __name__ == '__main__':
    main()
