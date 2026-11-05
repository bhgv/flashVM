"""Independent bounded wire-derived call semantics; never grants owning execution."""
from pathlib import Path
import hashlib
import json
import struct
import sys
from peer_call_goldens import sections, UnsupportedExtension
from peer_call_binding import bindings, bind_payloads, string_catalog, UnsupportedSchema
from peer_call_schema import metadata
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
    except UnsupportedExtension as failure:
        return dict(result, outer='canonical', stage='mandatory-extension',
                    status='unsupported', reason=str(failure),
                    required_extensions=failure.required_extensions)
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


def inspect_pair(consumer, provider):
    """Match every import to one explicit provider after structural validation.

    Inputs are immutable images. Publish only ordinals and descriptor coordinates
    after all imports match. No provider search, code pairing or runtime construction.
    """
    result = {'owning_execution_supported': False}
    payloads = []
    for role, image in (('consumer', consumer), ('provider', provider)):
        report = inspect_image(image)
        result[role + '_sha256'] = report['sha256']
        if report.get('structural_bindings') != 'passed':
            return dict(result, status=report['status'], stage=report['stage'],
                        reason=report['reason'], image_role=role)
        payloads.append({kind: image[offset:offset+size]
                         for kind, _, offset, size in sections(image)})

    consumer_payload, provider_payload = payloads
    imported = bindings(consumer_payload[5], False) if 5 in consumer_payload else []
    if not imported:
        return dict(result, status='unmatched', stage='provider-match', reason='consumer has no imports')
    exported = bindings(provider_payload[6], True) if 6 in provider_payload else []
    consumer_names = string_catalog(consumer_payload[7])
    provider_names = string_catalog(provider_payload[7])
    module_name = provider_names[metadata(provider_payload[1])['name']]
    symbols = {provider_names[binding['symbol']]: (ordinal, binding)
               for ordinal, binding in enumerate(exported)}
    matches = []
    for ordinal, binding in enumerate(imported):
        match = symbols.get(consumer_names[binding['symbol']])
        if consumer_names[binding['module']] != module_name:
            return dict(result, status='unmatched', stage='provider-match',
                        import_ordinal=ordinal, reason='provider module name differs')
        if match is None:
            return dict(result, status='unmatched', stage='provider-match',
                        import_ordinal=ordinal, reason='provider symbol is missing')
        export_ordinal, export = match
        if binding['signature'] != export['signature']:
            return dict(result, status='unmatched', stage='provider-match',
                        import_ordinal=ordinal, reason='provider signature differs')
        matches.append({'import_ordinal': ordinal, 'export_ordinal': export_ordinal,
                        'entry': export['entry'], 'frame': export['frame']})
    return dict(result, status='matched', stage='provider-match', matches=matches)


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


def main(args):
    """Inspect a corpus directory or match two explicitly named image files."""
    if len(args) == 1 and args[0] != '--pair':
        return inspect(Path(args[0]))
    if len(args) != 3 or args[0] != '--pair':
        raise SystemExit('usage: peer_call_semantics.py CORPUS | --pair CONSUMER PROVIDER')
    images = []
    for name in args[1:]:
        with Path(name).open('rb') as source:
            image = source.read(MAX_IMAGE+1)
        if len(image) > MAX_IMAGE:
            raise SystemExit('offline image ceiling exceeded: ' + name)
        images.append(image)
    return inspect_pair(*images)


if __name__ == '__main__':
    print(json.dumps(main(sys.argv[1:]), indent=2))
