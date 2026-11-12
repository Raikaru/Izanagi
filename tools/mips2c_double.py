#!/usr/bin/env python3
"""Reproduce the retail double-libm cores as native, integer-only C.

Like mips2c.py's float translations, every branch/delay slot and operation is
retained. Unlike the float cores, these functions call the EE's fp-bit software
arithmetic; using host double operators here would change their truncation,
denormal and NaN behaviour. No instruction bytes are embedded in the output.
"""
import argparse
from pathlib import Path
import re
import struct
import subprocess

FUNCTIONS = ((0x441cb0, 0x441cf0), (0x441d70, 0x441ef0),
             (0x4470f0, 0x4476b0), (0x4476b0, 0x447b88),
             (0x447b88, 0x447d00), (0x447d00, 0x448a00),
             (0x448a00, 0x448d10), (0x44b310, 0x44b340))
HELPERS = {0x44e0a8: 'func_0044e0a8', 0x44e100: 'dpsub',
           0x44e168: 'func_0044e168', 0x44e3b8: 'dpdiv',
           0x44e608: 'dpcmp', 0x44e658: 'litodp'}
REGS = 'at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp s8 ra'.split()
BRANCHES = {'b', 'beq', 'bne', 'beqz', 'bnez', 'bltz', 'bgez', 'blez', 'bgtz',
            'beql', 'bnel', 'beqzl', 'bnezl', 'bltzl', 'bgezl', 'blezl', 'bgtzl'}


def instructions(objdump, elf, lo, hi):
    text = subprocess.run([objdump, '-D', '-z', '-mmips:5900', f'--start-address={lo:#x}',
                           f'--stop-address={hi:#x}', str(elf)], check=True, text=True, capture_output=True).stdout
    result = {}
    for line in text.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+[0-9a-f]+\s+(\S+)\s*(.*)', line)
        if m:
            result[int(m[1], 16)] = (m[2], [v.strip() for v in m[3].split('#')[0].split(',') if v.strip()])
    if len(result) != (hi - lo) // 4:
        raise ValueError(f'{lo:08x}: incomplete disassembly')
    return result


def integer(s):
    return int(s, 0)


def operand(s):
    return 'UINT64_C(0)' if s == 'zero' else s


def simple(m, o):
    r = [operand(v) for v in o]
    if m == 'nop':
        return ';'
    if m == 'move':
        return f'{r[0]} = {r[1]};'
    if m == 'li':
        return f'{r[0]} = (uint64_t)(int64_t){integer(o[1])};'
    if m == 'lui':
        return f'{r[0]} = D_SEXT32(UINT32_C(0x{integer(o[1]) << 16:08x}));'
    if m in ('addiu', 'addu', 'subu'):
        if o[0] == 'sp':
            return '; /* native frame replaces EE sp adjustment */'
        rhs = str(integer(o[2])) if m == 'addiu' else r[2]
        op = '-' if m == 'subu' else '+'
        return f'{r[0]} = D_SEXT32((uint32_t){r[1]} {op} (uint32_t)({rhs}));'
    if m == 'negu':
        return f'{r[0]} = D_SEXT32(UINT32_C(0) - (uint32_t){r[1]});'
    if m in ('and', 'or', 'xor', 'nor', 'andi', 'ori', 'xori'):
        op = {'and': '&', 'or': '|', 'xor': '^', 'nor': '|', 'andi': '&', 'ori': '|', 'xori': '^'}[m]
        rhs = str(integer(o[2])) if m.endswith('i') else r[2]
        expr = f'({r[1]} {op} {rhs})'
        return f'{r[0]} = {"~" if m == "nor" else ""}{expr};'
    if m in ('slt', 'sltu', 'slti', 'sltiu'):
        cast = '(uint64_t)' if 'u' in m else '(int64_t)'
        rhs = str(integer(o[2])) if 'i' in m else r[2]
        return f'{r[0]} = {cast}{r[1]} < {cast}({rhs});'
    if m in ('sll', 'srl', 'sra', 'sllv', 'srlv', 'srav'):
        shift = f'({r[2]} & 31)' if m.endswith('v') else str(integer(o[2]))
        cast = 'int32_t' if m.startswith('sra') else 'uint32_t'
        op = '<<' if m.startswith('sll') else '>>'
        return f'{r[0]} = D_SEXT32(({cast}){r[1]} {op} {shift});'
    if m in ('dsll', 'dsrl', 'dsra', 'dsll32', 'dsrl32', 'dsra32'):
        shift = integer(o[2]) + (32 if m.endswith('32') else 0)
        cast = '(int64_t)' if m.startswith('dsra') else '(uint64_t)'
        op = '<<' if m.startswith('dsll') else '>>'
        return f'{r[0]} = {cast}{r[1]} {op} {shift};'
    if m in ('movn', 'movz'):
        return f'if ({r[2]} {"!=" if m == "movn" else "=="} 0) {r[0]} = {r[1]};'
    if m in ('ld', 'lw', 'lwu', 'sd', 'sw'):
        offset, base = re.fullmatch(r'(-?\d+)\((\w+)\)', o[1]).groups()
        offset = int(offset)
        if base == 'sp':
            addr = f'frame + {offset}'
            if m in ('sd', 'sw'):
                if m == 'sd':
                    return f'memcpy({addr}, &{r[0]}, 8);'
                return f'dlibm_store32({addr}, (uint32_t){r[0]});'
            if m == 'ld':
                return f'memcpy(&{r[0]}, {addr}, 8);'
            expr = f'dlibm_load32({addr})'
            return f'{r[0]} = {"D_SEXT32(" + expr + ")" if m == "lw" else expr};'
        if m != 'ld':
            raise ValueError(f'unsupported nonconstant memory operation {m} {o}')
        return f'{r[0]} = dlibm_constant({base} + {offset});'
    raise ValueError(f'unsupported instruction {m} {o}')


def condition(m, o):
    r = [operand(v) for v in o]
    if m == 'b':
        return '1'
    base = m[:-1] if m.endswith('l') else m
    if base in ('beq', 'bne'):
        return f'{r[0]} {"==" if base == "beq" else "!="} {r[1]}'
    op = {'beqz': '==', 'bnez': '!=', 'bltz': '<', 'bgez': '>=', 'blez': '<=', 'bgtz': '>'}[base]
    return f'(int64_t){r[0]} {op} 0'


def call(addr):
    name = HELPERS.get(addr, f'dlibm_{addr:08x}')
    if addr == 0x44e658:
        return f'{name}((int32_t)a0)'
    return f'{name}(a0, a1)'


def lift(lo, hi, ins):
    used = set()
    for m, operands in ins.values():
        used.update(v for v in operands if v in REGS and v not in ('a0', 'a1'))
    out = [f'/* Retail {lo:08x}..{hi:08x}; one label per EE instruction. */',
           f'static uint64_t dlibm_{lo:08x}(uint64_t a0, uint64_t a1)', '{']
    out.append('    uint64_t ' + ', '.join(r + ' = 0' for r in REGS if r in used) + ';')
    if any('(sp)' in ','.join(operands) for m, operands in ins.values()):
        out.append('    unsigned char frame[128];')
    for addr, (m, o) in ins.items():
        out.append(f'L_{addr:08x}:')
        if m in BRANCHES:
            target = int(o[-1], 16)
            if not lo <= target < hi:
                raise ValueError(f'{addr:08x}: out-of-function conditional branch')
            slot = simple(*ins[addr + 4])
            if m == 'b':
                out.append(f'    {slot} goto L_{target:08x};')
            else:
                if m.endswith('l'):
                    out.append(f'    if ({condition(m, o)}) {{ {slot} goto L_{target:08x}; }}')
                else:
                    out.append(f'    {{ int taken = ({condition(m, o)}); {slot} if (taken) goto L_{target:08x}; }}')
                out.append(f'    goto L_{addr + 8:08x};')
        elif m in ('jal', 'j', 'jr'):
            out.append('    ' + simple(*ins[addr + 4]))
            if m == 'jr':
                if o != ['ra']:
                    raise ValueError(f'{addr:08x}: computed jump')
                out.append('    return v0;')
            else:
                target = int(o[0], 16)
                if m == 'j' and lo <= target < hi:
                    out.append(f'    goto L_{target:08x};')
                elif m == 'j':
                    out.append(f'    return {call(target)};')
                else:
                    out.append(f'    v0 = {call(target)};')
                    out.append(f'    goto L_{addr + 8:08x};')
        else:
            out.append('    ' + simple(m, o))
    out.append('}')
    return '\n'.join(out)


PRELUDE = '''/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* fdlibm notice, for the code these functions translate:
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this software is freely granted,
 * provided that this notice is preserved. */
/* Literal translations of retail double-libm and its scaling/sign callees.
 * Generated by tools/mips2c_double.py from GNU R5900 objdump, not host libm.
 * All floating arithmetic goes through the already asm-verified fp-bit cores.
 * Constants below are mathematical coefficients represented by their exact bits.
 */
#include <stdint.h>
#include <string.h>
#include "ps2_fpbit.h"
#include "p4pc/ee_dlibm.h"

#define D_SEXT32(x) ((uint64_t)(int64_t)(int32_t)(uint32_t)(x))
static uint32_t dlibm_load32(const void *address)
{
    uint32_t value;
    memcpy(&value, address, sizeof value);
    return value;
}
static void dlibm_store32(void *address, uint32_t value)
{
    memcpy(address, &value, sizeof value);
}
'''


def elf_reader(path):
    data = path.read_bytes()
    offset = struct.unpack_from('<I', data, 28)[0]
    size, count = struct.unpack_from('<HH', data, 42)
    segments = []
    for i in range(count):
        kind, off, va, _, length, *_ = struct.unpack_from('<8I', data, offset + i * size)
        if kind == 1:
            segments.append((va, off, length))
    def word(address):
        for va, off, length in segments:
            if va <= address <= va + length - 8:
                return struct.unpack_from('<Q', data, off + address - va)[0]
        raise ValueError(f'unmapped constant {address:08x}')
    return word


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', required=True, type=Path)
    parser.add_argument('--objdump', default='mipsel-linux-gnu-objdump')
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    word = elf_reader(args.elf)
    all_ins = [(lo, hi, instructions(args.objdump, args.elf, lo, hi)) for lo, hi in FUNCTIONS]
    constants = set()
    for lo, hi, ins in all_ins:
        for addr, (m, o) in ins.items():
            if m == 'ld' and '(t7)' in o[1]:
                offset = int(o[1].split('(')[0])
                if offset:
                    constants.add(0x750000 + offset)
    # pow's three two-entry breakpoint tables use a dynamic 0/8-byte index.
    constants.update(range(0x755990, 0x7559c0, 8))
    base = min(constants)
    count = (max(constants) - base) // 8 + 1
    out = [PRELUDE, f'static const uint64_t dlibm_coefficients[{count}] = {{']
    for address in sorted(constants):
        out.append(f'    [{(address - base) // 8}] = UINT64_C(0x{word(address):016x}), /* {address:08x} */')
    out.extend(['};', '', 'static inline uint64_t dlibm_constant(uint64_t address)', '{',
                f'    return dlibm_coefficients[((uint32_t)address - 0x{base:08x}u) >> 3];', '}', ''])
    for lo, hi in FUNCTIONS:
        out.append(f'static uint64_t dlibm_{lo:08x}(uint64_t, uint64_t);')
    for lo, hi, ins in all_ins:
        out.extend(['', lift(lo, hi, ins)])
    for lo, hi in FUNCTIONS:
        if 0x447000 <= lo < 0x449000:
            argc = 'uint64_t x, uint64_t y' if lo == 0x447d00 else 'uint64_t x'
            passed = 'x, y' if lo == 0x447d00 else 'x, 0'
            out.append(f'\nuint64_t func_{lo:08x}({argc})\n{{\n    return dlibm_{lo:08x}({passed});\n}}')
    args.out.write_text('\n'.join(out) + '\n', encoding='utf-8')
    print(f'Generated {args.out}: {sum(len(ins) for _, _, ins in all_ins)} instructions, {len(constants)} exact mathematical constants')


if __name__ == '__main__':
    main()
