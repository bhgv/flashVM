"""Independent bounded metadata/type/string schema inspection; no call admission."""
from pathlib import Path
import hashlib
import json
import struct
import sys
from peer_call_goldens import sections


class Reader:
    """Borrow immutable bytes; consume bounded BE words/spans, with exact finish."""

    def __init__(self, data):
        """data is a complete borrowed section, never a native pointer/Heap."""
        self.data = data
        self.at = 0

    def take(self, count):
        """Return count bytes or reject truncation before advancing the cursor."""
        if count < 0 or count > len(self.data)-self.at:
            raise ValueError('truncated section')
        value = self.data[self.at:self.at+count]
        self.at += count
        return value

    def word(self):
        """Return one unsigned BE32; take supplies the bounds check."""
        return struct.unpack('>I', self.take(4))[0]

    def finish(self):
        """Reject any unconsumed bytes; return None on exact consumption."""
        if self.at != len(self.data):
            raise ValueError('trailing section bytes')


def metadata(data):
    """Check metadata field presence/profile, not entry/type/call-role binding."""
    source = Reader(data)
    name, profile, size = source.word(), source.word(), source.word()
    if profile not in (0, 1):
        raise ValueError('module profile')
    if size:
        source.word()
    if profile:
        source.word()
        source.word()
    source.finish()
    return {'name': name, 'profile': profile, 'data_bytes': size}


def types(data):
    """Check complete map8 descriptors independent of private np0/type budgets.

    Managed maps and >64 descriptors may be structurally legal. No executable
    frame-kind, local-kind or root/safepoint authority is inferred from a map.
    """
    source = Reader(data)
    count = source.word()
    if count > (len(data)-4)//8:
        raise ValueError('type count')
    managed = 0
    for _ in range(count):
        size, align = source.word(), source.word()
        if align not in (1, 2, 4, 8) or size % align or (not size and align != 1):
            raise ValueError('type alignment')
        bitmap = source.take((size+63)//64)
        for index, value in enumerate(bitmap):
            if value and align < 8:
                raise ValueError('unaligned reference map')
            for bit in range(8):
                if value & (128 >> bit) and (index*8+bit)*8+8 > size:
                    raise ValueError('reference outside object')
        managed += bool(any(bitmap))
    source.finish()
    return {'count': count, 'managed_maps': managed}


def strings(data):
    """Consume pool/record counts and raw BE Rune32, preserving every payload bit.

    Duplicate contents and non-Unicode Rune32 values are legal wire values here.
    No UTF normalization, nil sentinel or native string allocation is inferred.
    """
    source = Reader(data)
    count = source.word()
    if count > (len(data)-4)//4:
        raise ValueError('string count')
    runes = 0
    for _ in range(count):
        length = source.word()
        source.take(4*length)
        runes += length
    source.finish()
    return {'count': count, 'runes': runes}


def inspect(corpus):
    """Check manifest byte identities and partial schemas; never consume locals.

    corpus is an explicit immutable peer directory. Return per-case classifications.
    Code/exports, cross-section bindings and typed execution remain unvalidated.
    """
    manifest_bytes = (corpus/'manifest.json').read_bytes()
    manifest = json.loads(manifest_bytes)
    cases = []
    for item in manifest['cases']:
        name = item['name']
        if Path(name).name != name:
            raise ValueError('nonlocal name')
        image = bytes.fromhex((corpus/(name+'.hex')).read_text())
        if len(image) != item['bytes'] or hashlib.sha256(image).hexdigest() != item['sha256']:
            raise ValueError('byte identity: '+name)
        try:
            directory = sections(image)
        except ValueError as failure:
            cases.append({'name': name, 'outer': 'malformed', 'reason': str(failure),
                          'partial_schema': 'not reached', 'execution': 'not admitted'})
            continue
        payload = {kind: image[offset:offset+size] for kind, _, offset, size in directory}
        row = {'name': name, 'outer': 'canonical', 'execution': 'not evaluated; no admission granted',
               'unvalidated': ['code/exports schema', 'cross-section bindings', 'typed call roles/lifecycle']}
        try:
            row.update(metadata=metadata(payload[1]), types=types(payload[3]), strings=strings(payload[7]))
        except (ValueError, KeyError) as failure:
            row.update(partial_schema='malformed', reason=str(failure))
        else:
            row['partial_schema'] = 'metadata/types/strings valid'
        cases.append(row)
    return {'manifest_sha256': hashlib.sha256(manifest_bytes).hexdigest(),
            'scope': 'independent partial schema, no code/call acceptance', 'cases': cases}


def self_test():
    """Check independent legal boundaries and malformed section refusal cases."""
    pack = lambda *values: struct.pack('>'+'I'*len(values), *values)
    if metadata(pack(0, 0, 0))['profile'] != 0 or types(pack(1, 8, 8)+b'\x80')['managed_maps'] != 1:
        raise RuntimeError('legal metadata/managed type')
    if types(pack(65)+(pack(8, 8)+b'\0')*65)['count'] != 65:
        raise RuntimeError('private type budget became schema authority')
    if strings(pack(2, 1, 0xffffffff, 1, 0xffffffff))['runes'] != 2:
        raise RuntimeError('lossless duplicate Rune32')
    failures = [(metadata, pack(0, 2, 0)), (metadata, pack(0, 0, 0, 0)),
                (types, pack(1, 8, 3)+b'\0'), (types, pack(1, 8, 8)),
                (types, pack(1, 8, 8)+b'\x40'), (strings, pack(1, 2, 7)),
                (strings, pack(0, 0))]
    for function, image in failures:
        try:
            function(image)
        except ValueError:
            continue
        raise RuntimeError('malformed schema accepted: '+function.__name__)
    print('PASS partial schema: four legal boundaries, seven malformed refusals', file=sys.stderr)


if __name__ == '__main__':
    self_test()
    if sys.argv[1:] != ['--self-test']:
        print(json.dumps(inspect(Path(sys.argv[1])), indent=2))
