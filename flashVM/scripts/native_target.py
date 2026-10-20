"""Bind native evidence to active CMake metadata and the running reporter ABI.

Cross-build/device validation is separate. A compilation fixture or an emulator
must not be relabeled as native foreign-platform execution.
"""
from pathlib import Path
import platform
import re
import struct


def architecture(value):
    """Normalize OS/compiler aliases without conflating pointer widths."""
    aliases = {'amd64': 'x86_64', 'x64': 'x86_64', 'x86_64': 'x86_64',
               'arm64': 'aarch64', 'aarch64': 'aarch64',
               'x86': 'x86', 'i386': 'x86', 'i486': 'x86', 'i586': 'x86', 'i686': 'x86',
               'arm': 'arm', 'armv7l': 'arm', 'armv7': 'arm',
               'riscv64': 'riscv64', 'riscv32': 'riscv32'}
    if not isinstance(value, str) or value.lower() not in aliases:
        raise ValueError('unrecognized native architecture: ' + str(value))
    return aliases[value.lower()]


def active_directory(build, cache):
    """Select current CMake metadata; stale directories do not change the decision."""
    parts = [cache.get('CMAKE_CACHE_' + part + '_VERSION', '')
             for part in ('MAJOR', 'MINOR', 'PATCH')]
    if any(not isinstance(part, str) or not re.fullmatch(r'[0-9]+', part) for part in parts):
        raise ValueError('invalid active CMake version')
    return Path(build) / 'CMakeFiles' / '.'.join(parts)


def validate_native_target(build, cache, toolchains):
    """Require literal host/target fields, no cross mode and matching native ABI.

    A 32-bit target from a 64-bit reporting process deliberately requires a
    separate validation setup. This routine does not execute CMake input.
    """
    path = active_directory(build, cache) / 'CMakeSystem.cmake'
    fields = ('CMAKE_CROSSCOMPILING', 'CMAKE_HOST_SYSTEM_NAME', 'CMAKE_HOST_SYSTEM_PROCESSOR',
              'CMAKE_SYSTEM_NAME', 'CMAKE_SYSTEM_PROCESSOR')
    values = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        match = re.fullmatch(r'\s*set\(\s*(CMAKE_[A-Z0-9_]+)\s+"([^"\r\n]*)"\s*\)\s*', line)
        if match and match[1] in fields:
            if match[1] in values:
                raise ValueError('duplicate CMake system identity')
            values[match[1]] = match[2]
    if set(values) != set(fields) or values['CMAKE_CROSSCOMPILING'] != 'FALSE':
        raise ValueError('native validation requires complete non-cross CMake metadata')
    system = platform.system()
    if system not in ('Linux', 'Windows', 'Darwin'):
        raise ValueError('unsupported desktop OS; use device-specific evidence')
    if values['CMAKE_SYSTEM_NAME'] != system or values['CMAKE_HOST_SYSTEM_NAME'] != system:
        raise ValueError('CMake host/target OS differs from running reporter')
    machine = architecture(platform.machine())
    for field in ('CMAKE_HOST_SYSTEM_PROCESSOR', 'CMAKE_SYSTEM_PROCESSOR'):
        if architecture(values[field]) != machine:
            raise ValueError('CMake host/target architecture differs from running reporter')
    width = struct.calcsize('P')
    expected_width = 8 if machine in ('x86_64', 'aarch64', 'riscv64') else 4
    if width != expected_width:
        raise ValueError('reporter pointer ABI differs from machine architecture')
    for language in ('C', 'CXX'):
        identity = toolchains[language]
        if identity['pointer_size'] != width:
            raise ValueError(language + ' pointer ABI differs from running reporter')
        if identity['architecture'] and architecture(identity['architecture']) != machine:
            raise ValueError(language + ' compiler architecture differs from running reporter')
    return {'system': system, 'machine': machine, 'pointer_size': width, 'metadata': str(path),
            'scope': 'same-OS same-ABI native execution; no cross/emulator/device certification'}
