"""Inspect explicit peer call corpus without granting executable call admission."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile


class UnsupportedExtension(ValueError):
    """The envelope is canonical and requires features unavailable to this reader."""

    def __init__(self, records):
        """Keep validated extension identity and version requirements for reports."""
        super().__init__('mandatory extension is not implemented')
        self.required_extensions = records


def sections(image):
    """Independently inspect this corpus's canonical outer framing only.

    image is immutable bytes. Return directory tuples or raise ValueError before
    any payload interpretation. This is not a full codec, type/call validator or
    runtime loader; trusted manifest locals are deliberately never consumed.
    """
    if len(image) < 32 or image[:8] != b'Dis64\r\n\x1a':
        raise ValueError('magic/header')
    major, minor, header, abi, flags, extensions, count = struct.unpack_from('>HHIIIII', image, 8)
    if (major, minor, abi, flags) != (1, 0, 1, 0) or not 0 < count <= 1024:
        raise ValueError('profile/directory count')
    manifest_end = header - 12*count
    if not 32 <= manifest_end <= header <= len(image) or extensions > (manifest_end-32)//9:
        raise ValueError('directory extent')
    cursor = 32
    previous = b''
    required_extensions = []
    for _ in range(extensions):
        if manifest_end-cursor < 8:
            raise ValueError('extension record extent')
        name_bytes, major, minimum_minor, reserved = struct.unpack_from('>HHHH', image, cursor)
        cursor += 8
        if reserved or not 1 <= name_bytes <= min(255, manifest_end-cursor):
            raise ValueError('extension name extent/reserved field')
        name = image[cursor:cursor+name_bytes]
        if not re.fullmatch(rb'[a-z][a-z0-9._-]*', name) or name <= previous:
            raise ValueError('extension name/order')
        required_extensions.append({'name': name.decode('ascii'), 'major': major,
                                    'minimum_minor': minimum_minor})
        previous = name
        cursor += name_bytes
    if cursor != manifest_end:
        raise ValueError('extension/directory boundary')
    directory = [struct.unpack_from('>HHII', image, cursor+12*index) for index in range(count)]
    seen = set()
    end = header
    for kind, flags, offset, length in directory:
        if offset != end or length > len(image)-offset:
            raise ValueError('section geometry')
        if 1 <= kind <= 8:
            minimum = 1 if kind <= 3 else 4 if kind >= 5 else 0
            if flags != 1 or length < minimum:
                raise ValueError('standard section flags/length')
        elif flags != 0:
            raise ValueError('optional section flags')
        if 1 <= kind <= 8 or kind in (32768, 32769):
            if kind in seen:
                raise ValueError('duplicate section')
            seen.add(kind)
        end = offset+length
    if end != len(image):
        raise ValueError('truncated/trailing bytes')
    if not {1, 2, 3} <= seen:
        raise ValueError('missing mandatory section')
    if extensions:
        raise UnsupportedExtension(required_extensions)
    return directory


def main():
    """Use explicit matching inspector/corpus; preserve bytes and rejected output.

    SHA256 is byte identity, never authentication or executable type authority.
    Every corpus image must remain refused by the current scalar execution gate.
    """
    inferred = '--infer' in sys.argv[1:]
    inspector, corpus = (Path(value).resolve() for value in sys.argv[1:] if value != '--infer')
    manifest = json.loads((corpus / 'manifest.json').read_text())
    cases = manifest['cases']
    names = {case['name'] for case in cases}
    expected = 14 if inferred else 25
    required = {'1-normal', '1-unused', '1-overlap', '1-reinterpret'} if inferred else {
        'answer42', 'chain42', 'reordered42', 'divide-zero'}
    if len(cases) != expected or len(names) != expected or not required <= names:
        raise RuntimeError('peer corpus population changed; review revision explicitly')
    if inferred and ('locals' in manifest or any('locals' in case for case in cases)):
        raise RuntimeError('inference manifest unexpectedly supplies local authority')
    accepted_framing = 0
    rejected_framing = 0
    with tempfile.TemporaryDirectory(prefix='peer file-call framing ') as directory:
        work = Path(directory)
        image_path = work / 'input.dis64'
        output_path = work / 'untouched.dis64'
        for case in cases:
            name = case['name']
            if Path(name).name != name:
                raise RuntimeError('nonlocal corpus name')
            image = bytes.fromhex((corpus / (name+'.hex')).read_text())
            if len(image) != case['bytes'] or hashlib.sha256(image).hexdigest() != case['sha256']:
                raise RuntimeError('peer byte identity mismatch: '+name)
            image_path.write_bytes(image)
            try:
                directory = sections(image)
            except ValueError:
                framing = False
                rejected_framing += 1
            else:
                framing = True
                accepted_framing += 1
            inspected = subprocess.run([str(inspector), str(image_path)],
                                       capture_output=True, text=True, timeout=20)
            if framing:
                if inspected.returncode:
                    raise RuntimeError('framing disagreement: '+name+inspected.stderr)
                report = json.loads(inspected.stdout)
                actual = [(entry['kind'], entry['flags'], entry['offset'], entry['bytes'])
                          for entry in report['sections']]
                if actual != directory or report['execution_supported'] or not report['envelope_only']:
                    raise RuntimeError('directory/authority disagreement: '+name)
            elif inspected.returncode != 1 or 'invalid envelope' not in inspected.stderr:
                raise RuntimeError('outer refusal disagreement: '+name)
            output_path.write_bytes(b'KEEP')
            refused = subprocess.run([str(inspector), str(image_path), '--pilot', '--rewrite', str(output_path)],
                                     capture_output=True, text=True, timeout=20)
            if refused.returncode != 1 or output_path.read_bytes() != b'KEEP':
                raise RuntimeError('closed scalar admission/output changed: '+name+refused.stderr)
            diagnostic = 'unsupported or invalid scalar pilot payloads' if framing else 'invalid envelope'
            if diagnostic not in refused.stderr:
                raise RuntimeError('refusal stage disagreement: '+name)
    expected_framing = (14, 0) if inferred else (23, 2)
    if (accepted_framing, rejected_framing) != expected_framing:
        raise RuntimeError('unexpected outer framing population')
    print('PASS peer'+('0101' if inferred else '0100')+':'+str(expected)+' SHA256/length identities;'+
          str(accepted_framing)+' independent outer directories;'+str(rejected_framing)+' framing refusals;'+
          str(expected)+' current scalar refusals preserve output; no call execution/type-authority claim')


if __name__ == '__main__':
    main()
