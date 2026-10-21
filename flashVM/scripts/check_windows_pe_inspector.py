#!/usr/bin/env python3
"""Independent synthetic PE bytes exercise rejection paths without executing a target."""
from pathlib import Path
import struct
import tempfile
from inspect_windows_pe import check_pe


def fixture():
    """Construct minimal mapped headers, one section and one import-name descriptor."""
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 132, 0x8664, 1, 0, 0, 0, 240, 2)
    struct.pack_into('<H', data, 152, 0x20b)
    struct.pack_into('<I', data, 152 + 60, 512)
    struct.pack_into('<I', data, 152 + 108, 16)
    struct.pack_into('<II', data, 152 + 120, 0x1000, 40)
    struct.pack_into('<IIII', data, 392 + 8, 512, 0x1000, 512, 512)
    struct.pack_into('<IIIII', data, 512, 0, 0, 0, 0x1040, 0x1050)
    data[576:589] = b'KERNEL32.dll\0'
    return data


def main():
    """Require valid OS imports and explicit failures for malformed/wrong-profile images."""
    with tempfile.TemporaryDirectory(prefix='flashvm-pe-') as directory:
        path = Path(directory) / 'fixture.exe'
        good = fixture()
        path.write_bytes(good)
        assert check_pe(path) == ['kernel32.dll']
        bad = []
        bad.append(good[:20])
        for offset, fmt, value in [(132, '<H', 0xaa64), (150, '<H', 0x2002),
                                   (152, '<H', 0x10b), (260, '<I', 17),
                                   (512 + 12, '<I', 0x3000), (400, '<I', 0xffffffff),
                                   (152 + 112 + 13 * 8, '<I', 1)]:
            data = good.copy()
            struct.pack_into(fmt, data, offset, value)
            bad.append(data)
        data = good.copy()
        data[576:589] = b'libbad00.dll\0'
        bad.append(data)
        data = good.copy()
        struct.pack_into('<II', data, 272, 0x1000, 20)  # No terminating descriptor in directory.
        bad.append(data)
        data = good.copy()
        struct.pack_into('<I', data, 524, 0x11ff)
        data[1023] = ord('X')  # Name crosses into virtual-only/unmapped bytes.
        bad.append(data)
        for index, data in enumerate(bad):
            path.write_bytes(data)
            try:
                check_pe(path)
            except ValueError:
                pass
            else:
                raise AssertionError('accepted malformed vector ' + str(index))
        print('PASS PE32+ x64 OS-import fixture and', len(bad), 'independent rejection vectors')


if __name__ == '__main__':
    main()
