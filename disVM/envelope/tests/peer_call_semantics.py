"""Independent bounded wire-derived call semantics; never grants owning execution."""
from pathlib import Path
import hashlib
import json
import struct
import sys
from peer_call_goldens import sections
from peer_call_binding import bind_payloads, UnsupportedSchema
from peer_call_roles import catalog, Refusal, Unsupported
from peer_call_flow import check_flow

LIMITS = {2: 8192, 3: 4096, 5: 1024, 6: 1024, 7: 4096}  # Offline checker ceilings, not wire/private policy.
MAX_IMAGE = 4*1024*1024  # Explicit byte ceiling; exceeds every exchanged golden.


def inspect_image(image):
    """Borrow immutable image bytes; return staged verdict/facts with execution closed.

    All roles and facts come from sections. No manifest metadata is an input.
    Unsupported facilities are not called malformed or silently admitted.
    """
    result = {'sha256': hashlib.sha256(image).hexdigest(), 'bytes': len(image),
              'execution': 'closed; no loader/runtime admission', 'owning_execution_supported': False,
              'required_execution_mode': 'explicit immutable dis64-calls; NOT activated by this checker',
              'unproved': ['owning allocation/GC/root/frame/code lifetimes', 'runtime arithmetic safety/termination',
                           'non-straight-line or cross-module execution', 'unused descriptor semantic roles']}
    if len(image) > MAX_IMAGE:
        return dict(result, stage='resource-limit', status='unsupported', reason='offline image ceiling')
    try:
        directory = sections(image)
    except ValueError as failure:
        return dict(result, stage='outer', status='malformed', reason=str(failure))
    result['outer'] = 'canonical'
    payload = {kind: image[offset:offset+size] for kind, _, offset, size in directory}
    try:
        for kind, maximum in LIMITS.items():
            if kind in payload and len(payload[kind]) >= 4 and struct.unpack_from('>I', payload[kind])[0] > maximum:
                raise Unsupported('resource-limit', 'offline section-count ceiling')
        bind_payloads(payload)
    except UnsupportedSchema as failure:
        return dict(result, stage='schema/binding', status='unsupported', reason=str(failure))
    except Unsupported as failure:
        return dict(result, stage=failure.stage, status='unsupported', reason=str(failure))
    except (ValueError, KeyError) as failure:
        return dict(result, stage='schema/binding', status='malformed', reason=str(failure))
    result['structural_bindings'] = 'passed'
    try:
        world = catalog(payload)
        facts = check_flow(world)
    except Unsupported as failure:
        return dict(result, stage=failure.stage, status='unsupported', reason=str(failure))
    except Refusal as failure:
        return dict(result, stage=failure.stage, status='bounded-refusal', reason=str(failure))
    result.update(stage='bounded-semantics', status='bounded-checks-passed', functions=facts,
                  module_entry=world['entry'], used_frame_type_ids=sorted({f['frame'] for f in world['functions'].values()}),
                  unanalysed_type_ids=world['unanalysed_type_ids'])
    return result


def inspect(corpus):
    """Verify names/length/SHA identity only, then inspect each immutable image.

    Sidecar private_accept, locals, result, policy and descriptions are ignored.
    Return one full verdict per image; no runtime/native pointer is constructed.
    """
    if (corpus/'manifest.json').stat().st_size > MAX_IMAGE:
        raise ValueError('offline manifest byte ceiling')
    manifest_bytes = (corpus/'manifest.json').read_bytes()
    manifest = json.loads(manifest_bytes)
    rows = []
    names = set()
    if len(manifest['cases']) > 1024:
        raise ValueError('offline corpus case ceiling')
    for identity in manifest['cases']:
        name = identity['name']
        if not name or Path(name).name != name or '/' in name or '\\' in name or name in names:
            raise ValueError('nonlocal or repeated corpus name')
        names.add(name)
        path = corpus/(name+'.hex')
        if path.stat().st_size > MAX_IMAGE*4:
            raise ValueError('offline hex-text byte ceiling')
        image = bytes.fromhex(path.read_text())
        if len(image) != identity['bytes'] or hashlib.sha256(image).hexdigest() != identity['sha256']:
            raise ValueError('byte identity: '+name)
        rows.append(dict(inspect_image(image), name=name))
    sources = ('peer_call_semantics.py', 'peer_call_roles.py', 'peer_call_flow.py',
               'peer_call_binding.py', 'peer_call_schema.py', 'peer_call_goldens.py')
    return {'scope': 'independent bounded wire-derived roles/init/result/LEA; NO owning admission',
            'manifest_sha256': hashlib.sha256(manifest_bytes).hexdigest(),
            'checker_sources': {name: hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest()
                                for name in sources},
            'limits': {'image_bytes': MAX_IMAGE, 'section_counts': LIMITS, 'signature_parameters': 256,
                       'inferred_slots_per_function': 1024, 'corpus_cases': 1024}, 'cases': rows}


if __name__ == '__main__':
    print(json.dumps(inspect(Path(sys.argv[1])), indent=2))
