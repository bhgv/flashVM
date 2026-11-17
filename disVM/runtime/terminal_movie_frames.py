# Copyright (c) 2026 bhgv
# EN: Independent Python integer/CRC wire oracles for real guest movie-service FD requests.
# RU: Independent Python integer/CRC wire oracles для настоящих guest movie-service FD requests.
import zlib


# EN/RU: Encode canonical positive signed-magnitude integer, without the tested native codec.
# Кодировать canonical positive signed-magnitude integer без проверяемого native codec.
def integer(value):
    groups = []
    while value > 63:
        groups.insert(0, value % 128)
        value //= 128
    groups.insert(0, value)
    groups[-1] |= 128
    return bytes(groups)


# EN/RU: Compute exact standard S-record CRC geometry with Python's independent zlib implementation.
# Рассчитать exact S-record CRC geometry через независимую Python zlib implementation.
def record(tag, payload):
    body = integer(tag) + payload
    length = integer(len(body))
    mask = (1 << min(32, 6 + 7 * (len(length)-1))) - 1
    return integer(zlib.crc32(length+body) & mask) + length + body


# EN/RU: Write test-only Limbo hex constants; no implementation headers or native helper calls.
# Записать test-only Limbo hex constants без implementation headers/native helper calls.
def prepare(work):
    identity = b'flashvm.movie'
    binding = record(0, b''.join(integer(v) for v in (1, 1, 1, 1, len(identity))) + identity)
    path = b'/dev/keyboard'
    request = bytes([1, len(path), 0, 0, 0]) + path
    begin = record(1, bytes([2])+path)
    # Independent raw SWF8 fixture, matching examples/player/embedded_movie/movie.h.
    swf = bytes.fromhex('465753082c000000800000c80000007d00001801004302305070440b0e3e0300c6040000001000f040000000')
    if len(swf) != 44:
        raise RuntimeError('raw SWF oracle length')
    (work/'startup.swf').write_bytes(swf)
    startup = b'/startup.swf'
    fresh_request = record(1, bytes([1, 1, len(startup), 0, 0, 0])+startup)
    fresh_reply = record(1, bytes([2])+startup)+record(1, bytes([3])+swf)+record(1, bytes([4, 0]))
    records = dict(FreshRequest=binding+fresh_request, FreshReply=binding+fresh_reply,
                   CodeRequest=binding+record(1, bytes([1])+request),
                   DataRequest=binding+record(1, bytes([1, 2])+request[1:]),
                   CodeReply=binding+begin+record(1, bytes([3])+b'P'),
                   DataReply=binding+begin+record(1, bytes([3])+b'T'),
                   DeniedReply=binding+record(1, bytes([4, 1])))
    (work/'movieframes.m').write_text('\n'.join(name+': con "'+data.hex()+'";' for name, data in records.items())+'\n')
