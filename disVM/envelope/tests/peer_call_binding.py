"""Independent bounded code/exports schemas and static ID/range binding only."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import tempfile
from peer_call_schema import Reader, metadata, types, strings
from peer_call_goldens import sections

STATIC_REFS = {4: 'call-pc', 5: 'frame-type', 13: 'jump-pc'}  # Existing agreed ICALL/IFRAME/IJMP IDs.


class UnsupportedSchema(ValueError):
    """An unsupported schema/address form must not become a malformed-wire claim."""


def operand(source):
    """Consume signed7/14/30 mathematically, accepting existing nonminimal forms."""
    first = source.take(1)[0]
    length = 1 if first < 128 else 2 if first < 192 else 4
    value = first & (127 if length == 1 else 63)
    for byte in source.take(length-1):
        value = value*256+byte
    bits = {1: 7, 2: 14, 4: 30}[length]
    return value-(1 << bits) if value & (1 << (bits-1)) else value


def code_schema(data):
    """Decode exact direct framing plus the already used indirect FP destination.

    Other indirect forms stop as unsupported, never fall back to a legacy reader.
    Opcode operand semantics and function ownership are not validated here.
    """
    source = Reader(data)
    count = source.word()
    if count > (len(data)-4)//2:
        raise ValueError('instruction count/extent')
    result = []
    for _ in range(count):
        op, mode = source.take(2)
        sm, dm, mm = (mode >> 3) & 7, mode & 7, mode & 192
        row = {'opcode': op, 'mode': mode}
        if dm == 5:
            if mm or sm not in (1, 2):
                raise UnsupportedSchema('unsupported indirect code form')
            row.update(source=operand(source), destination=operand(source), secondary=operand(source))
        elif sm <= 3 and dm <= 3:
            if mm:
                row['middle'] = operand(source)
            if sm != 3:
                row['source'] = operand(source)
            if dm != 3:
                row['destination'] = operand(source)
        else:
            raise UnsupportedSchema('unsupported code addressing form')
        result.append(row)
    source.finish()
    return result


def signature(data):
    """Consume version1 scalar kinds, keeping Bool distinct from Int32.

    No private parameter budget, local declarations or executable type proof.
    Return immutable result/parameter tuple; unknown version is unsupported.
    """
    source = Reader(data)
    if source.word() != 1:
        raise UnsupportedSchema('unsupported signature version')
    result, count = source.word(), source.word()
    if result not in range(5) or count > (len(data)-12)//4:
        raise ValueError('signature kind/count')
    params = tuple(source.word() for _ in range(count))
    if any(value not in (1, 2, 3, 4) for value in params):
        raise ValueError('signature parameter kind')
    source.finish()
    return result, params


def bindings(data, exports):
    """Decode export/import records; absent fields gain no wire sentinel."""
    source = Reader(data)
    count = source.word()
    if count > (len(data)-4)//(28 if exports else 24):
        raise ValueError('binding count/extent')
    rows = []
    for _ in range(count):
        if exports:
            row = {'symbol': source.word(), 'entry': source.word(), 'frame': source.word()}
        else:
            row = {'module': source.word(), 'symbol': source.word()}
        row['signature'] = signature(source.take(source.word()))
        rows.append(row)
    source.finish()
    return rows


def type_catalog(data):
    """Return structurally checked descriptor extents, without assigning kinds."""
    types(data)
    source = Reader(data)
    rows = []
    for _ in range(source.word()):
        size, align = source.word(), source.word()
        source.take((size+63)//64)
        rows.append({'bytes': size, 'alignment': align})
    source.finish()
    return rows


def string_catalog(data):
    """Resolve ordinal names by lossless Rune32 content, including duplicate IDs."""
    strings(data)
    source = Reader(data)
    rows = []
    for _ in range(source.word()):
        rows.append(tuple(source.word() for _ in range(source.word())))
    source.finish()
    return rows


def bind_payloads(payload):
    """Validate structural IDs/ranges from raw sections, without sidecar authority.

    Static immediate FRAME IDs and CALL/JMP PCs are checked, not callable target
    ownership or dynamic references. Module-data extent is bounded by descriptor;
    exact allocation layout, entry/export frame agreement and roles stay open.
    """
    module = metadata(payload[1])
    info = Reader(payload[1])
    info.take(12)
    data_type = info.word() if module['data_bytes'] else None
    entry, frame = (info.word(), info.word()) if module['profile'] else (None, None)
    info.finish()
    catalog = type_catalog(payload[3])
    pool = string_catalog(payload[7])
    instructions = code_schema(payload[2])
    exported = bindings(payload[6], True) if 6 in payload else []
    imported = bindings(payload[5], False) if 5 in payload else []
    if module['name'] >= len(pool):
        raise ValueError('module name ID')
    if data_type is not None and (data_type >= len(catalog) or module['data_bytes'] > catalog[data_type]['bytes']):
        raise ValueError('module data ID/range')

    def frame_ok(index):
        """index is an ordinal; return structural frame range/alignment validity."""
        return index < len(catalog) and catalog[index]['bytes'] >= 64 and catalog[index]['alignment'] == 8

    if entry is not None and (entry >= len(instructions) or not frame_ok(frame)):
        raise ValueError('module entry/frame ID/range')
    names = set()
    for item in exported:
        if item['symbol'] >= len(pool) or item['entry'] >= len(instructions) or not frame_ok(item['frame']):
            raise ValueError('export name/entry/frame ID/range')
        name = pool[item['symbol']]
        if name in names:
            raise ValueError('duplicate export content')
        names.add(name)
    imports = {}
    for item in imported:
        if item['module'] >= len(pool) or item['symbol'] >= len(pool):
            raise ValueError('import name ID')
        key = pool[item['module']], pool[item['symbol']]
        if key in imports and imports[key] != item['signature']:
            raise ValueError('conflicting import signature')
        imports[key] = item['signature']
    refs = []
    for pc, item in enumerate(instructions):
        kind = STATIC_REFS.get(item['opcode'])
        if kind == 'frame-type' and ((item['mode'] >> 3) & 7) == 2:
            target = item['source']
            if target < 0 or not frame_ok(target):
                raise ValueError('static FRAME type ID/range')
            refs.append({'instruction': pc, 'kind': kind, 'target': target})
        elif kind in ('call-pc', 'jump-pc') and (item['mode'] & 7) == 2:
            target = item['destination']
            if target < 0 or target >= len(instructions):
                raise ValueError('static control PC range')
            refs.append({'instruction': pc, 'kind': kind, 'target': target})
    return {'instructions': len(instructions), 'exports': len(exported), 'imports': len(imported),
            'types': len(catalog), 'strings': len(pool), 'checked_static_refs': refs}


def inspect(corpus):
    """Return immutable corpus identities and staged classifications; never execute.

    Manifest locals/private_accept/private budgets are deliberately not consumed.
    Structural success is not a confirmation of private semantic acceptance.
    """
    data = (corpus/'manifest.json').read_bytes()
    manifest = json.loads(data)
    rows = []
    for item in manifest['cases']:
        name = item['name']
        if Path(name).name != name:
            raise ValueError('nonlocal name')
        image = bytes.fromhex((corpus/(name+'.hex')).read_text())
        if len(image) != item['bytes'] or hashlib.sha256(image).hexdigest() != item['sha256']:
            raise ValueError('byte identity: '+name)
        row = {'name': name, 'execution': 'unvalidated; no admission granted',
               'unvalidated': ['opcode/local-slot semantics', 'function ownership/call roles',
                               'exact module-data layout', 'entry/export frame agreement', 'owning execution']}
        try:
            directory = sections(image)
        except ValueError as failure:
            row.update(stage='outer', status='malformed', reason=str(failure))
        else:
            payload = {kind: image[offset:offset+size] for kind, _, offset, size in directory}
            try:
                row['decoded'] = bind_payloads(payload)
            except UnsupportedSchema as failure:
                row.update(stage='schema/binding', status='unsupported', reason=str(failure))
            except (ValueError, KeyError) as failure:
                row.update(stage='schema/binding', status='malformed', reason=str(failure))
            else:
                row.update(stage='schema/binding', status='structural IDs/ranges valid')
        rows.append(row)
    return {'manifest_sha256': hashlib.sha256(data).hexdigest(), 'scope': 'independent bounded code/bindings; no semantic execution proof',
            'cases': rows}


def self_test():
    """Exercise signed compact forms and independently built schema/ref mutations."""
    pack = lambda *v: struct.pack('>'+'I'*len(v), *v)
    for encoded, value in [(b'\x3f', 63), (b'\x40', -64), (b'\xbf\xff', -1),
                           (b'\xdf\xff\xff\xff', (1 << 29)-1),
                           (b'\xe0\0\0\0', -(1 << 29)), (b'\x80\0', 0), (b'\xc0\0\0\0', 0)]:
        source = Reader(encoded)
        if operand(source) != value:
            raise RuntimeError('signed operand oracle')
        source.finish()
    sig = pack(1, 0, 0)
    export = pack(1, 1, 0, 0, len(sig))+sig
    valid = {1: pack(0, 1, 0, 0, 0), 2: pack(1)+b'\x0f\x1b',
             3: pack(1, 64, 8)+b'\0', 6: export, 7: pack(2, 1, ord('m'), 1, ord('f'))}
    if bind_payloads(valid)['exports'] != 1 or bindings(pack(0), False):
        raise RuntimeError('legal schema oracle')
    mutations = []
    for section, body in [(2, pack(2)+b'\x0f\x1b'), (2, pack(1)+b'\x0f\x1b\0'),
                          (6, pack(1, 2, 0, 0, len(sig))+sig),
                          (6, pack(1, 1, 1, 0, len(sig))+sig),
                          (6, pack(1, 1, 0, 1, len(sig))+sig),
                          (1, pack(0, 1, 0, 1, 0)),
                          (6, pack(2)+export[4:]+export[4:]),
                          (2, pack(1)+b'\x05\x11\x01\x28'),
                          (2, pack(1)+b'\x04\x0a\x28\x01'),
                          (1, pack(2, 1, 0, 0, 0)),
                          (1, pack(0, 1, 8, 1, 0, 0)),
                          (1, pack(0, 1, 128, 0, 0, 0)),
                          (3, pack(1, 32, 8)+b'\0'),
                          (3, pack(1, 64, 4)+b'\0'),
                          (5, pack(1, 2, 1, len(sig))+sig),
                          (6, pack(1, 1, 0, 0, 16)+pack(1, 1, 1, 0))]:
        mutations.append(dict(valid))
        mutations[-1][section] = body
    for candidate in mutations:
        try:
            bind_payloads(candidate)
        except ValueError:
            continue
        raise RuntimeError('malformed ID/range accepted')
    for body in [pack(1)+b'\0\x06', pack(1)+b'\0\x20']:
        try:
            code_schema(body)
        except UnsupportedSchema:
            continue
        raise RuntimeError('unsupported mode became valid/malformed')
    # Build a complete minimal image independently; manifest labels stay inert.
    offset = 32+12*len(valid)
    directory = bytearray()
    content = bytearray()
    for kind, body in sorted(valid.items()):
        directory += struct.pack('>HHII', kind, 1, offset, len(body))
        content += body
        offset += len(body)
    image = b'Dis64\r\n\x1a'+struct.pack('>HHIIIII', 1, 0, 32+12*len(valid), 1, 0, 0, len(valid))+directory+content
    with tempfile.TemporaryDirectory(prefix='binding identity ') as temp:
        root = Path(temp)
        (root/'self.hex').write_text(image.hex())
        item = {'name': 'self', 'bytes': len(image), 'sha256': hashlib.sha256(image).hexdigest(),
                'locals': [], 'private_accept': False}
        (root/'manifest.json').write_text(json.dumps({'cases': [item]}))
        before = inspect(root)['cases']
        item.update(locals={'poison': [999, -1]}, private_accept=True, production_accept=True)
        (root/'manifest.json').write_text(json.dumps({'private_policy': {'types': 0}, 'cases': [item]}))
        if inspect(root)['cases'] != before:
            raise RuntimeError('manifest policy acquired authority')
        item['sha256'] = '0'*64
        (root/'manifest.json').write_text(json.dumps({'cases': [item]}))
        try:
            inspect(root)
        except ValueError:
            pass
        else:
            raise RuntimeError('identity mutation accepted')
    print('PASS independent code/binding schemas: compact boundaries,16 malformed refs, unsupported modes, inert labels', file=sys.stderr)


if __name__ == '__main__':
    self_test()
    if sys.argv[1:] != ['--self-test']:
        print(json.dumps(inspect(Path(sys.argv[1])), indent=2))
