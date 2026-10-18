#!/usr/bin/env python3
"""Execute synthetic-address backend fixtures with Unicorn (optional test dependency).

Generate fixtures with flashvm_tests_jit_backends DIRECTORY, then pass DIRECTORY here.
This checks machine ABI/control flow, not full foreign-OS FlashVM execution.
"""
import argparse
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_ARCH_ARM, UC_ARCH_ARM64, UC_ARCH_RISCV
from unicorn import UC_MODE_32, UC_MODE_64, UC_MODE_ARM, UC_MODE_RISCV32, UC_MODE_RISCV64, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ESI
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R4
from unicorn.arm64_const import UC_ARM64_REG_SP, UC_ARM64_REG_PC, UC_ARM64_REG_LR, UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X19, UC_ARM64_REG_X18, UC_ARM64_REG_X20, UC_ARM64_REG_X29
from unicorn.riscv_const import UC_RISCV_REG_SP, UC_RISCV_REG_PC, UC_RISCV_REG_RA, UC_RISCV_REG_A0, UC_RISCV_REG_A1, UC_RISCV_REG_S0

TARGETS = [
    ('x86', UC_ARCH_X86, UC_MODE_32, 4, UC_X86_REG_ESP, UC_X86_REG_EIP, None, UC_X86_REG_EAX, None, UC_X86_REG_ESI),
    ('arm', UC_ARCH_ARM, UC_MODE_ARM, 4, UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R4),
    ('arm64', UC_ARCH_ARM64, UC_MODE_ARM, 8, UC_ARM64_REG_SP, UC_ARM64_REG_PC, UC_ARM64_REG_LR, UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X19),
    ('riscv', UC_ARCH_RISCV, UC_MODE_RISCV32, 4, UC_RISCV_REG_SP, UC_RISCV_REG_PC, UC_RISCV_REG_RA, UC_RISCV_REG_A0, UC_RISCV_REG_A1, UC_RISCV_REG_S0),
    ('riscv64', UC_ARCH_RISCV, UC_MODE_RISCV64, 8, UC_RISCV_REG_SP, UC_RISCV_REG_PC, UC_RISCV_REG_RA, UC_RISCV_REG_A0, UC_RISCV_REG_A1, UC_RISCV_REG_S0),
    ('macos-arm64', UC_ARCH_ARM64, UC_MODE_ARM, 8, UC_ARM64_REG_SP, UC_ARM64_REG_PC, UC_ARM64_REG_LR, UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X19),
]


def check(spec, binary, fail, variant=""):
    name, arch, mode, width, sp, pc, lr, a0, a1, saved = spec
    uc = Uc(arch, mode)
    base, helpers, stack, end, state = 0x1000000, 0x200000, 0x300000, 0x400000, 0x12345678
    if variant == "high":
        helpers += 0x100000000
        state = 0x123456789abcdef0
    uc.mem_map(base, (len(binary) + 0xffff) & ~0xffff)
    for address in (helpers, stack, end):
        uc.mem_map(address, 0x10000)
    uc.mem_write(base, binary)
    ret_instruction = {'x86': b'\xc3', 'arm': bytes.fromhex('1eff2fe1'),
                       'arm64': bytes.fromhex('c0035fd6'),
                       'macos-arm64': bytes.fromhex('c0035fd6'),
                       'riscv': bytes.fromhex('67800000'),
                       'riscv64': bytes.fromhex('67800000')}[name]
    if variant == "thumb":
        ret_instruction = bytes.fromhex('7047')
    for offset in (0, 0x100, 0x200):
        uc.mem_write(helpers + offset, ret_instruction)
    initial_sp = stack + 0x8000
    sentinel = 0x76543210
    uc.reg_write(saved, sentinel)
    if name == 'macos-arm64':
        for register, value in ((UC_ARM64_REG_X18, 0x1818181818181818),
                                (UC_ARM64_REG_X20, 0x2020202020202020),
                                (UC_ARM64_REG_X29, stack + 0x9000)):
            uc.reg_write(register, value)
    if name == 'x86':
        initial_sp -= 4
        uc.mem_write(initial_sp, struct.pack('<II', end, state))
    else:
        uc.reg_write(a0, state)
        uc.reg_write(lr, end)
    uc.reg_write(sp, initial_sp)
    calls = []
    predicates = 0

    def hook(machine, address, size, userdata):
        nonlocal predicates
        if not helpers <= address < helpers + 0x1000:
            return
        assert address in (helpers, helpers + 0x100, helpers + 0x200)
        current_sp = machine.reg_read(sp)
        if name == 'x86':
            ret, arg, operand = struct.unpack('<III', bytes(machine.mem_read(current_sp, 12)))
            assert current_sp % 16 == 12
        else:
            ret, arg, operand = machine.reg_read(lr), machine.reg_read(a0), machine.reg_read(a1) & 0xffffffff
            assert current_sp % (8 if name == 'arm' else 16) == 0
            if name == 'riscv64':
                extended = operand if operand < 0x80000000 else operand | 0xffffffff00000000
                assert machine.reg_read(a1) == extended, (name, 'incorrect psABI sign extension')
        if name == 'macos-arm64':
            frame = machine.reg_read(UC_ARM64_REG_X29)
            assert frame == current_sp + 16
            parent, caller = struct.unpack('<QQ', bytes(machine.mem_read(frame, 16)))
            assert parent == stack + 0x9000 and caller == end
            assert machine.reg_read(UC_ARM64_REG_X18) == 0x1818181818181818
            assert machine.reg_read(a1) == operand, 'unsigned operand must be zero extended'
            assert base <= ret < base + len(binary)
        assert arg == state, (name, hex(arg))
        if address == helpers + 0x200:
            predicates += 1
            result = int(predicates == 2)
        else:
            calls.append(operand)
            result = 7 if fail and operand == 2 else 0
        # Clobber argument registers to ensure each call reinstalls state and operands.
        if a1 is not None:
            machine.reg_write(a1, 0xdeadbeef)
        machine.reg_write(a0, result)
        # Execute the actual ISA return instruction; do not redirect the emulator PC.

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(base, end, count=10000)
    assert uc.reg_read(pc) == end, (name, 'did not return')
    assert calls == ([0xffffffff, 0x7fffffff, 0x80000000, 1, 2] if fail else [0xffffffff, 0x7fffffff, 0x80000000, 1, 2, 1, 3]), (name, calls)
    assert uc.reg_read(a0) == (7 if fail else 0)
    assert uc.reg_read(saved) == sentinel
    if name == 'macos-arm64':
        assert uc.reg_read(UC_ARM64_REG_X18) == 0x1818181818181818
        assert uc.reg_read(UC_ARM64_REG_X20) == 0x2020202020202020
        assert uc.reg_read(UC_ARM64_REG_X29) == stack + 0x9000
    assert uc.reg_read(sp) == initial_sp + (4 if name == 'x86' else 0)
    print(name, variant, 'error exit' if fail else 'forward/backward branches', 'OK')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    if not __debug__:
        parser.error("Python assertions must be enabled; unset PYTHONOPTIMIZE and do not use -O")
    for index, spec in zip((1, 2, 3, 4, 5, 9), TARGETS):
        variants = ["", "far"]
        if spec[0] in ("arm64", "riscv64", "macos-arm64"):
            variants.append("high")
        if spec[0] == "arm":
            variants.append("thumb")
        for variant in variants:
            suffix = "-" + variant if variant else ""
            binary = (args.directory / ('backend-%d%s.bin' % (index, suffix))).read_bytes()
            for fail in (False, True):
                check(spec, binary, fail, variant)


if __name__ == '__main__':
    main()
