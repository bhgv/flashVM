"""Inspect explicit peer call corpus without granting executable call admission."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import sys
import tempfile


def sections(image):
    """Independently inspect this corpus's canonical outer framing only.

    image is immutable bytes. Return directory tuples or raise ValueError before
    any payload interpretation. This is not a full codec, type/call validator or
    runtime loader; trusted manifest locals are deliberately never consumed.
    """
    if len(image) < 32 or image[:8] != b'Dis64\r\n\x1a':
        raise ValueError('magic/header')
    major, minor, header, abi, flags, reserved, count = struct.unpack_from('>HHIIIII', image, 8)
    if (major, minor, abi, flags, reserved) != (1, 0, 1, 0, 0) or not 0 < count <= 1024:
        raise ValueError('profile/directory count')
    if header != 32+12*count or header > len(image):
        raise ValueError('directory extent')
    directory = [struct.unpack_from('>HHII', image, 32+12*index) for index in range(count)]
    if len({kind for kind, _, _, _ in directory}) != count:
        raise ValueError('duplicate section')
    end = header
    for kind, mandatory, offset, length in directory:
        if not 1 <= kind <= 7 or mandatory != 1 or offset != end or length > len(image)-offset:
            raise ValueError('section geometry')
        end = offset+length
    if end != len(image):
        raise ValueError('truncated/trailing bytes')
    return directory


def main():
    """Use explicit matching inspector/corpus; preserve bytes and rejected output.

    SHA256 is byte identity, never authentication or executable type authority.
    Every corpus image must remain refused by the current scalar execution gate.
    """
    inspector, corpus = (Path(value).resolve() for value in sys.argv[1:])
    manifest = json.loads((corpus / 'manifest.json').read_text())
    cases = manifest['cases']
    names = {case['name'] for case in cases}
    if len(cases) != 25 or len(names) != 25 or not {'answer42', 'chain42', 'reordered42', 'divide-zero'} <= names:
        raise RuntimeError('peer corpus population changed; review revision explicitly')
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
    if (accepted_framing, rejected_framing) != (23, 2):
        raise RuntimeError('unexpected outer framing population')
    print('PASS peer0100:25 SHA256/length identities;23 independent outer directories;2 framing refusals;'
          '25 current scalar refusals preserve output; no call execution/type-authority claim')


if __name__ == '__main__':
    main()
