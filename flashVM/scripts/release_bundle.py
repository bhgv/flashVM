#!/usr/bin/env python3
"""Create deterministic portable native-evidence ZIPs, never source/Git archives.

Exports only validated report/JUnit/inventory files plus a content manifest.
Compilation artifacts, arbitrary command logs and foreign absolute paths are not
opened. The output is a transport format, not a signed acceptance certificate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import zipfile
from ctest_evidence import read_bounded
from release_matrix import artifact, report_cells, skip_rule


ARTIFACTS = ('ctest-results.xml', 'test-inventory.log', 'test-inventory-after.log')


def create_bundle(report, commit, version, output, acknowledgements=None):
    """Validate a staged snapshot before atomically publishing a reproducible ZIP.

    Fixed timestamps, permissions, member order and stored compression avoid
    host filesystem and zlib-version differences. Failed exports leave any old
    archive untouched; callers must check the returned process status.
    """
    report, output = Path(report).resolve(), Path(output).resolve()
    if output.is_relative_to(report.parent):
        raise ValueError('archive output must be outside the input bundle')
    accepted = report_cells(report, commit, version, acknowledgements or {})
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary_zip = None
    with tempfile.TemporaryDirectory(prefix='.native-evidence-', dir=output.parent) as temporary:
        stage = Path(temporary)
        (stage / 'platform-evidence.json').write_bytes(read_bounded(report))
        for key, _ in accepted:
            profile = key[2]
            (stage / profile).mkdir()
            for name in ARTIFACTS:
                (stage / profile / name).write_bytes(read_bounded(artifact(report.parent, profile, name)))
        # A concurrent edit between validation and copying cannot bypass checks.
        report_cells(stage / 'platform-evidence.json', commit, version, acknowledgements or {})
        paths = sorted(p for p in stage.rglob('*') if p.is_file())
        members = []
        for path in paths:
            data = read_bounded(path)
            members.append({'name': path.relative_to(stage).as_posix(), 'size': len(data),
                            'sha256': hashlib.sha256(data).hexdigest()})
        manifest = {'schema': 1, 'kind': 'flashvm-native-evidence', 'commit': commit,
                    'version': version, 'members': members,
                    'scope': 'portable local test evidence; not signed/full roadmap acceptance'}
        (stage / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n',
                                              encoding='utf-8')
        try:
            with tempfile.NamedTemporaryFile(prefix='.' + output.name + '.', dir=output.parent,
                                             delete=False) as stream:
                temporary_zip = Path(stream.name)
            with zipfile.ZipFile(temporary_zip, 'w', compression=zipfile.ZIP_STORED) as archive:
                for path in sorted(p for p in stage.rglob('*') if p.is_file()):
                    info = zipfile.ZipInfo(path.relative_to(stage).as_posix(), (1980, 1, 1, 0, 0, 0))
                    info.create_system = 3
                    info.external_attr = 0o100644 << 16
                    archive.writestr(info, read_bounded(path))
            os.replace(str(temporary_zip), str(output))
        finally:
            if temporary_zip is not None:
                temporary_zip.unlink(missing_ok=True)
    return manifest


def main():
    """Keep exports explicit; return failure without claiming new evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--allow-skip', action='append', default=[], metavar='OS:ARCH:PROFILE:TEST')
    args = parser.parse_args()
    try:
        rules = {}
        for value in args.allow_skip:
            key, name = skip_rule(value)
            rules.setdefault(key, []).append(name)
        manifest = create_bundle(args.report, args.commit, args.version, args.output, rules)
    except (OSError, ValueError, TypeError, KeyError, zipfile.BadZipFile) as error:
        print(str(error), file=sys.stderr)
        return 1
    print(str(args.output) + ': ' + str(len(manifest['members'])) + ' validated evidence files')
    return 0


if __name__ == '__main__':
    sys.exit(main())
