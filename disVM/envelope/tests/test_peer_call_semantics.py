"""Independent synthetic wire mutations and semantic-omission oracle controls."""
from pathlib import Path
import argparse
import copy
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import peer_call_flow
from peer_call_binding import bind_payloads
from peer_call_semantics import inspect_image, inspect_pair, inspect


def pack(*values):
    """Return owned canonical BE32 bytes for synthetic scalar fields."""
    return struct.pack('>'+'I'*len(values), *values)


def compact(value):
    """Encode signed7/14/30 independently for the small synthetic writer."""
    if -64 <= value < 64:
        return bytes([value&127])
    if -8192 <= value < 8192:
        return struct.pack('>H', 0x8000|(value&0x3fff))
    if -(1 << 29) <= value < (1 << 29):
        return struct.pack('>I', 0xc0000000|(value&0x3fffffff))
    raise ValueError('synthetic compact extent')


def instruction(opcode, mode, *operands):
    """Return raw opcode/mode and ordered compact operands; no semantic inference."""
    return bytes([opcode, mode])+b''.join(compact(value) for value in operands)


def fixture():
    """Build a raw root-to-leaf scalar call, without any sidecar local declarations."""
    rows = [instruction(45,17,42,64), instruction(5,17,1,40),
            instruction(45,13,64,40,64), instruction(39,13,104,40,32),
            instruction(4,10,40,7), instruction(45,9,104,80), instruction(15,27),
            instruction(45,13,64,32,0), instruction(12,27)]
    root_sig = pack(1,0,0)
    helper_sig = pack(1,1,1,1)
    payload = {1: pack(0,1,0,0,0), 2: pack(len(rows))+b''.join(rows),
               3: pack(2,112,8)+b'\0\0'+pack(88,8)+b'\0\0',
               6: pack(2,0,0,0,len(root_sig))+root_sig+pack(1,7,1,len(helper_sig))+helper_sig,
               7: pack(2,1,ord('m'),1,ord('f'))}
    return payload, rows


def envelope(payload, reverse=False, extra=(), extensions=()):
    """Build canonical outer directory independently; reverse changes physical order only."""
    sections = sorted(payload.items(), reverse=reverse)
    sections.extend(extra)
    manifest = b''.join(struct.pack('>HHHH', len(name), 1, 0, 0)+name for name in extensions)
    header = 32+len(manifest)+12*len(sections)
    offset = header
    directory = bytearray()
    content = bytearray()
    for kind, data in sections:
        directory += struct.pack('>HHII',kind,int(1 <= kind <= 8),offset,len(data))
        content += data
        offset += len(data)
    fixed = struct.pack('>HHIIIII', 1, 0, header, 1, 0, len(extensions), len(sections))
    return b'Dis64\r\n\x1a'+fixed+manifest+directory+content


def fail(label):
    """Terminate with exact oracle semantic10, never treating a crash as detection."""
    print('FAIL checkpoint oracle: '+label, file=sys.stderr)
    raise SystemExit(10)


def verdict(label, payload, status, stage=None):
    """Require independent expected label/status/stage, with execution always closed."""
    result = inspect_image(envelope(payload))
    if result['status'] != status or (stage and result['stage'] != stage) or not result['execution'].startswith('closed;'):
        fail(label)
    if result['owning_execution_supported'] or (status != 'bounded-checks-passed' and 'functions' in result):
        fail(label+' partial publication/admission')
    return result


def run_suite():
    """Exercise raw-byte role/init/width/LEA mutations, not production construction paths."""
    original, rows = fixture()
    cases = 0
    good = verdict('ordinary-call', original, 'bounded-checks-passed')
    reordered = inspect_image(envelope(original, True))
    if any(reordered[key] != good[key] for key in ('status','stage','functions')):
        fail('section-order')
    cases += 2

    optional = dict(original)
    optional.update({32768: b'debug', 32769: b'provenance', 65535: b'opaque'})
    annotated = verdict('optional-sections', optional, 'bounded-checks-passed')
    if annotated['functions'] != good['functions']:
        fail('opaque sections affected function facts')
    control = dict(original)
    control[8] = pack(0)
    verdict('unsupported-control', control, 'unsupported', 'control')
    required_opaque = bytearray(envelope(optional))
    struct.pack_into('>H', required_opaque, 32+12*(len(optional)-1)+2, 1)
    rejected = inspect_image(required_opaque)
    if (rejected['status'], rejected['stage']) != ('malformed', 'outer'):
        fail('unknown required section')
    missing_types = dict(original)
    del missing_types[3]
    verdict('missing-types', missing_types, 'malformed', 'outer')
    repeated_opaque = inspect_image(envelope(optional, extra=((65535, b'second opaque'),)))
    if repeated_opaque.get('functions') != good['functions']:
        fail('repeatable unknown optional kind')
    repeated_debug = inspect_image(envelope(optional, extra=((32768, b'second debug'),)))
    if (repeated_debug['status'], repeated_debug['stage']) != ('malformed', 'outer'):
        fail('singleton debug kind')
    cases += 6

    canonical_manifest = bytearray(envelope(original, extensions=(b'flashvm.a', b'flashvm.b')))
    struct.pack_into('>HH', canonical_manifest, 34, 513, 19)
    struct.pack_into('>HH', canonical_manifest, 34+8+len(b'flashvm.a'), 2, 65535)
    supported_framing = inspect_image(canonical_manifest)
    if ((supported_framing['status'], supported_framing['stage'], supported_framing['outer'])
            != ('unsupported', 'mandatory-extension', 'canonical')
            or supported_framing['owning_execution_supported'] or 'functions' in supported_framing
            or supported_framing.get('required_extensions') != [
                {'name': 'flashvm.a', 'major': 513, 'minimum_minor': 19},
                {'name': 'flashvm.b', 'major': 2, 'minimum_minor': 65535},
            ]):
        fail('canonical mandatory extension remains closed')
    cases += 1
    for names in ((b'Flashvm.a',), (b'flashvm.b', b'flashvm.a'),
                  (b'flashvm.a', b'flashvm.a'), (b'a'*256,)):
        invalid = inspect_image(envelope(original, extensions=names))
        if (invalid['status'], invalid['stage']) != ('malformed', 'outer'):
            fail('invalid extension name or order')
        cases += 1
    invalid_manifest = bytearray(canonical_manifest)
    struct.pack_into('>H', invalid_manifest, 38, 1)
    for image in (invalid_manifest, canonical_manifest[:-1]):
        invalid = inspect_image(image)
        if ((invalid['status'], invalid['stage']) != ('malformed', 'outer')
                or 'required_extensions' in invalid):
            fail('invalid extension record or section framing')
        cases += 1

    def changed(label, index, body, status='bounded-refusal', stage=None):
        """Replace one instruction while preserving all enclosing byte/section framing."""
        nonlocal cases
        payload = copy.deepcopy(original)
        code = rows.copy()
        code[index] = body
        payload[2] = pack(len(code))+b''.join(code)
        verdict(label, payload, status, stage)
        cases += 1

    changed('unready-result',0,instruction(45,9,104,64),stage='initialization')
    changed('wrong-prepared-type',1,instruction(5,17,0,40),stage='frame-signature')
    changed('wrong-result-tail',3,instruction(39,13,96,40,32),stage='lea-role')
    changed('missing-return',7,instruction(45,17,1,72),stage='return-initialization')
    changed('numeric-header',0,instruction(45,17,42,40),stage='local-role')
    changed('missing-argument',2,instruction(45,17,42,64),stage='argument-kind/init')
    changed('wrong-LEA-cell',3,instruction(39,13,104,40,40),stage='lea-role')
    changed('ordinary-LEA-confusion',3,instruction(39,9,104,32),stage='lea-role')
    changed('interior-target',4,instruction(4,10,40,8),stage='function-ownership')
    changed('argument-extra-slot',2,instruction(45,13,64,40,72),stage='argument-role')
    changed('partial-local-overlap',0,instruction(118,17,42,64),stage='local-kind')
    changed('unsupported-CFG',0,instruction(13,26,1),'unsupported','opcode/CFG')
    changed('last-shared-ISA-opcode',0,instruction(174,27),'unsupported','opcode/CFG')
    changed('first-reserved-ISA-opcode',0,instruction(175,27),'malformed','schema/binding')
    changed('reserved-ISA-opcode-with-indirect-FP',0,instruction(255,13,64,40,32),
            'malformed','schema/binding')
    changed('bad-terminal',8,instruction(15,27),stage='terminal-role')
    changed('fallthrough',8,instruction(45,17,1,72),stage='function-ownership')
    changed('new-FRAME-kills-old',5,instruction(5,17,1,40),stage='prepared-frame')
    changed('preparation-consumed',5,instruction(45,13,64,40,64),stage='prepared-frame')
    changed('unsupported-middle',0,instruction(45,81,1,42,64),'unsupported','middle-operand')
    for label, data, status, stage in [
        ('entry-interior',pack(0,1,0,1,0),'bounded-refusal','module-entry'),
        ('entry-frame',pack(0,1,0,0,1),'bounded-refusal','module-entry'),
        ('library',pack(0,0,0),'unsupported','module-role'),
        ('module-data',pack(0,1,112,0,0,0),'unsupported','module-data')]:
        payload = copy.deepcopy(original)
        payload[1] = data
        verdict(label,payload,status,stage)
        cases += 1
    for kind in (2,3):
        payload = copy.deepcopy(original)
        root_sig = pack(1,0,0)
        helper_sig = pack(1,kind,1,kind)
        payload[6] = pack(2,0,0,0,len(root_sig))+root_sig+pack(1,7,1,len(helper_sig))+helper_sig
        code = rows.copy()
        opcode = 118 if kind == 2 else 46
        code[0] = instruction(118,17,42,64)
        code[2] = instruction(opcode,13,64,40,64)
        code[5] = instruction(opcode,9,104,80)
        code[7] = instruction(opcode,13,64,32,0)
        if kind == 3:
            # A Real constant cannot be guessed from an Int32/Big immediate.
            payload[2] = pack(len(code))+b''.join(code)
            verdict('real-no-reinterpret',payload,'bounded-refusal','local-kind')
        else:
            payload[2] = pack(len(code))+b''.join(code)
            verdict('Big-width-not-WORD32',payload,'bounded-checks-passed')
            helper_sig = pack(1,1,1,kind)
            code[5] = rows[5]
            payload[2] = pack(len(code))+b''.join(code)
            payload[6] = pack(2,0,0,0,len(root_sig))+root_sig+pack(1,7,1,len(helper_sig))+helper_sig
            verdict('Big-result-to-Int32',payload,'bounded-refusal','result-kind')
            cases += 1
        cases += 1
    payload = copy.deepcopy(original)
    code = rows.copy()
    code[0] = instruction(45,17,42,68)
    code[2] = instruction(45,13,68,40,64)
    payload[2] = pack(len(code))+b''.join(code)
    verdict('four-byte-local-alignment',payload,'bounded-checks-passed')
    cases += 1
    for label, result_kind, params, source_offset, opcode in (
        ('Real-parameter-seed',3,(3,),64,46),
        ('packed-Int32-parameters',1,(1,1),68,45),
        ('mixed-width-parameters',1,(1,2,1),80,45)):
        payload = copy.deepcopy(original)
        root_sig = pack(1,0,0)
        helper_sig = pack(1,result_kind,len(params),*params)
        payload[6] = pack(2,0,0,0,len(root_sig))+root_sig+pack(1,1,1,len(helper_sig))+helper_sig
        code = [instruction(15,27),instruction(opcode,13,source_offset,32,0),instruction(12,27)]
        payload[2] = pack(len(code))+b''.join(code)
        verdict(label,payload,'bounded-checks-passed')
        cases += 1
    changed('RMW-read-before-write',0,instruction(67,17,2,64),stage='initialization')
    payload = copy.deepcopy(original)
    payload[4] = pack(0)
    verdict('DATA-not-silently-ignored',payload,'unsupported','module-data')
    cases += 1
    payload[1] = pack(0, 1, 16, 2, 0, 0)
    payload[3] = pack(3, 112, 8)+b'\0\0'+pack(88, 8)+b'\0\0'+pack(16, 8)+b'\x80'
    payload[4] = pack(2, 12, 4, 1, 8, 1, 42)
    verdict('DATA-valid-unordered-scalars', payload, 'unsupported', 'module-data')
    cases += 1
    for label, data in (
        ('DATA-mapped-reference', pack(1, 4, 1, 0)),
        ('DATA-Bool-domain', pack(1, 8, 4, 2)),
        ('DATA-overlap', pack(2, 8, 2, 0, 1, 12, 1, 0)),
        ('DATA-alignment', pack(1, 12, 3, 0, 0)),
        ('DATA-width-truncation', pack(1, 8, 3, 0)),
        ('DATA-unknown-kind', pack(1, 8, 5, 0)),
        ('DATA-out-of-extent', pack(1, 16, 1, 0)),
    ):
        payload[4] = data
        verdict(label, payload, 'malformed', 'schema/binding')
        cases += 1
    del payload[4]
    payload[1] = pack(0, 1, 8, 2, 0, 0)
    verdict('DATA-descriptor-extent-mismatch', payload, 'malformed', 'schema/binding')
    cases += 1
    payload = copy.deepcopy(original)
    payload[3] = pack(2,64,8)+b'\0'+pack(88,8)+b'\0\0'
    verdict('outgoing-tail-in-header',payload,'bounded-refusal','frame-role')
    cases += 1
    payload = copy.deepcopy(original)
    root_sig = pack(1,0,0)
    helper_sig = pack(1,1,1,4)
    payload[6] = pack(2,0,0,0,len(root_sig))+root_sig+pack(1,7,1,len(helper_sig))+helper_sig
    verdict('Bool-is-not-Int32',payload,'unsupported','signature-kind')
    cases += 1
    payload = copy.deepcopy(original)
    payload[5] = pack(0)
    verdict('empty-import-section',payload,'bounded-checks-passed')
    cases += 1
    payload = copy.deepcopy(original)
    del payload[6]
    verdict('absent-exports', payload, 'bounded-refusal', 'module-entry')
    cases += 1
    payload[1] = pack(0, 1, 8, 1, 0, 0)
    frame = instruction(11, 65, 0, 0, 64)
    call = instruction(9, 72, 0, 64, 0)
    payload[2] = pack(3)+frame+call+instruction(15, 27)
    payload[3] = pack(2, 80, 8)+b'\0\0'+pack(8, 8)+b'\x80'
    import_sig = pack(1, 0, 0)
    payload[5] = pack(1, 1, 1, len(import_sig))+import_sig
    result = verdict('paired-consumer-absent-exports', payload, 'unsupported', 'module-data')
    if result.get('structural_bindings') != 'passed':
        fail('paired consumer structural bindings')
    cases += 1
    imports = pack(2, 0, 0, len(import_sig))+import_sig+pack(0, 1, len(import_sig))+import_sig
    payload[5] = imports
    second_frame = instruction(11, 65, 1, 0, 72)
    second_call = instruction(9, 72, 1, 72, 0)
    paired = (frame, call, second_frame, second_call, instruction(15, 27))
    payload[2] = pack(len(paired))+b''.join(paired)
    verdict('paired-two-import-ordinals', payload, 'unsupported', 'module-data')
    refs = bind_payloads(payload)['checked_static_refs']
    if [(row['kind'], row['target']) for row in refs] != [
        ('mframe-import', 0), ('mcall-import', 0), ('mframe-import', 1), ('mcall-import', 1),
    ]:
        fail('paired immediate import reference order')
    cases += 1
    provider = dict(original)
    provider[6] = pack(2, 1, 7, 1, len(import_sig))+import_sig+pack(0, 0, 0, len(import_sig))+import_sig
    match = inspect_pair(envelope(payload), envelope(provider))
    if (match.get('matches') != [
        {'import_ordinal': 0, 'export_ordinal': 1, 'entry': 0, 'frame': 0},
        {'import_ordinal': 1, 'export_ordinal': 0, 'entry': 7, 'frame': 1},
    ] or match['owning_execution_supported']):
        fail('provider match uses names and preserves consumer ordinals')
    cases += 1
    provider[6] = original[6]
    mismatch = inspect_pair(envelope(payload), envelope(provider))
    if (mismatch['status'] != 'unmatched' or mismatch.get('import_ordinal') != 1
            or 'matches' in mismatch or mismatch['owning_execution_supported']):
        fail('provider signature mismatch publishes no partial match')
    cases += 1
    for index, replacement in ((2, instruction(11, 65, -1, 0, 72)),
                               (3, instruction(9, 72, 2, 72, 0))):
        broken = list(paired)
        broken[index] = replacement
        payload[2] = pack(len(broken))+b''.join(broken)
        verdict('paired-import-ordinal-range', payload, 'malformed', 'schema/binding')
        cases += 1
    for count, managed in ((65,False),(3,True)):
        payload = copy.deepcopy(original)
        payload[3] = pack(count)+original[3][4:]+b''.join(pack(8,8)+bytes([128 if managed else 0]) for _ in range(count-2))
        verdict('unused-descriptor-policy',payload,'bounded-checks-passed')
        cases += 1
    payload = copy.deepcopy(original)
    body = bytearray(payload[3])
    body[12] = 128
    payload[3] = bytes(body)
    verdict('managed-frame-needs-GC',payload,'unsupported','frame-map')
    cases += 1
    malformed = inspect_image(envelope(original)+b'\0')
    if malformed['stage'] != 'outer' or malformed['status'] != 'malformed':
        fail('outer-trailer')
    cases += 1
    return cases


def corpus_invariance(root):
    """Verify exact byte identities and that renamed/poisoned sidecars cannot change semantics."""
    baseline = inspect(root)
    with tempfile.TemporaryDirectory(prefix='inert call manifest ') as directory:
        target = Path(directory)
        manifest = {'private_policy': {'types': 0, 'instructions': 0}, 'locals': 'UNTRUSTED', 'cases': []}
        for index, row in enumerate(baseline['cases']):
            name = 'renamed'+str(index)
            image = bytes.fromhex((root/(row['name']+'.hex')).read_text())
            (target/(name+'.hex')).write_text(image.hex())
            manifest['cases'].append({'name':name,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),
                                      'private_accept': row['status'] != 'bounded-checks-passed',
                                      'locals': {'poison':'not executable authority'}, 'result': -999})
        (target/'manifest.json').write_text(json.dumps(manifest))
        altered = inspect(target)
        for before, after in zip(baseline['cases'],altered['cases']):
            if {key:value for key,value in before.items() if key != 'name'} != {key:value for key,value in after.items() if key != 'name'}:
                fail('sidecar-authority')
        manifest['cases'][0]['sha256'] = '0'*64
        (target/'manifest.json').write_text(json.dumps(manifest))
        try:
            inspect(target)
        except ValueError as failure:
            if 'byte identity:' not in str(failure):
                raise
        else:
            fail('identity-not-checked')
    return len(baseline['cases'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus',type=Path)
    parser.add_argument('--omit',choices=('initialization','frame-signature','lea-role','return-initialization'))
    args = parser.parse_args()
    if args.omit:
        original_require = peer_call_flow.require
        peer_call_flow.require = lambda valid,stage,reason: None if stage == args.omit else original_require(valid,stage,reason)
    count = run_suite()
    if args.corpus:
        identities = corpus_invariance(args.corpus)
        print('PASS',identities,'exact corpus identities; renamed/poisoned sidecars inert')
    if not args.omit:
        for stage, label in [('initialization','unready-result'),('frame-signature','wrong-prepared-type'),
                             ('lea-role','wrong-result-tail'),('return-initialization','missing-return')]:
            run = subprocess.run([sys.executable,str(Path(__file__).resolve()),'--omit',stage],capture_output=True,text=True)
            if run.returncode != 10 or 'FAIL checkpoint oracle: '+label not in run.stderr:
                raise RuntimeError('semantic omission control missed: '+run.stdout+run.stderr)
    print('PASS',count,'independent semantic wire cases and four exact-stage omission controls')
