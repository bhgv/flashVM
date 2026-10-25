#!/usr/bin/env python3
"""Inspect direct PE32+ x64 executable imports without executing foreign code.

This deliberately supports the linked executable subset used by the static Windows
profile. It rejects DLLs, delay imports, malformed bounds and unknown direct DLLs.
It cannot audit strings passed to LoadLibrary, transitive OS imports or runtime APIs.
"""
import argparse
from pathlib import Path
import struct

# Native Windows components remain dynamic OS dependencies of a statically linked player.
SYSTEM_DLLS = frozenset(
    """advapi32.dll bcrypt.dll combase.dll crypt32.dll d3d9.dll
 d3d11.dll dinput8.dll dsound.dll dwmapi.dll dxgi.dll gdi32.dll hid.dll imm32.dll
 kernel32.dll kernelbase.dll msvcrt.dll ntdll.dll ole32.dll oleaut32.dll opengl32.dll
 psapi.dll rpcrt4.dll setupapi.dll shcore.dll shell32.dll shlwapi.dll ucrtbase.dll
 user32.dll userenv.dll version.dll winmm.dll ws2_32.dll""".split()
)


def check_pe(path):
    """Return sorted direct OS DLL names after bounded PE/COFF geometry checks."""
    data = Path(path).read_bytes()

    def require(ok, message):
        if not ok:
            raise ValueError(str(path) + ": " + message)

    def unpack(fmt, offset):
        require(0 <= offset <= len(data) - struct.calcsize(fmt), "truncated structure")
        return struct.unpack_from(fmt, data, offset)

    require(len(data) >= 64 and data[:2] == b"MZ", "missing DOS header")
    (pe,) = unpack("<I", 60)
    require(pe >= 64 and data[pe : pe + 4] == b"PE\0\0", "missing PE signature")
    machine, sections, _, _, _, optional_size, flags = unpack("<HHIIIHH", pe + 4)
    require(
        machine == 0x8664 and flags & 2 and not flags & 0x2000,
        "expected x64 executable, not DLL/other CPU",
    )
    optional = pe + 24
    require(optional_size >= 112 and optional + optional_size <= len(data), "optional header size")
    require(unpack("<H", optional)[0] == 0x20B, "expected PE32+")
    (headers,) = unpack("<I", optional + 60)
    (directories,) = unpack("<I", optional + 108)
    require(2 <= directories <= (optional_size - 112) // 8, "data directory bounds")
    table = optional + optional_size
    require(
        0 < sections <= 96 and table + sections * 40 <= headers <= len(data), "section table bounds"
    )
    spans = []
    for index in range(sections):
        size, rva, raw_size, raw = unpack("<IIII", table + index * 40 + 8)
        require(raw_size == 0 or headers <= raw <= len(data) - raw_size, "section raw bounds")
        require(rva + max(size, raw_size) <= 0x100000000, "section RVA overflow")
        spans.append((rva, max(size, raw_size), raw, raw_size))
    spans.sort()
    require(all(a[0] + a[1] <= b[0] for a, b in zip(spans, spans[1:])), "overlapping RVA sections")

    def mapped(rva, size):
        require(0 < rva and size > 0 and rva + size <= 0x100000000, "invalid RVA range")
        if rva < headers:
            require(rva + size <= headers, "RVA crosses headers")
            return rva
        for base, _, raw, raw_size in spans:
            if base <= rva and rva - base + size <= raw_size:
                return raw + rva - base
        raise ValueError(str(path) + ": unmapped or virtual-only RVA")

    if directories > 13:
        require(unpack("<II", optional + 112 + 13 * 8) == (0, 0), "delay imports unsupported")
    imports, import_size = unpack("<II", optional + 112 + 8)
    require(imports and import_size >= 20, "missing import directory")
    offset = mapped(imports, import_size)
    names = set()
    terminated = False
    for cursor in range(offset, offset + import_size - 19, 20):
        descriptor = unpack("<IIIII", cursor)
        if descriptor == (0, 0, 0, 0, 0):
            terminated = True
            break
        require(descriptor[3] and descriptor[4], "invalid import descriptor")
        name = bytearray()
        for index in range(260):
            char = data[mapped(descriptor[3] + index, 1)]
            if not char:
                break
            require(32 < char < 127 and char not in b"/\\:", "invalid DLL name")
            name.append(char)
        else:
            raise ValueError(str(path) + ": unterminated DLL name")
        dll = name.decode("ascii").lower()
        api_set = dll.startswith(("api-ms-win-", "ext-ms-win-")) and dll.endswith(".dll")
        require(dll in SYSTEM_DLLS or api_set, "non-system direct import: " + dll)
        names.add(dll)
    require(terminated and names, "unterminated or empty import table")
    return sorted(names)


def main():
    """Print one checked executable identity and direct import list per supplied path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", nargs="+", type=Path)
    args = parser.parse_args()
    for path in args.artifacts:
        try:
            names = check_pe(path)
        except (OSError, ValueError) as error:
            parser.exit(1, str(error) + "\n")
        print(str(path) + ": PE32+ x64, direct OS imports: " + ", ".join(names))


if __name__ == "__main__":
    main()
