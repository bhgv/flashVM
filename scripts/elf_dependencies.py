"""Audit direct ELF dependencies of native CTest artifacts with GNU readelf.

This checks built artifacts rather than CMake link intentions. It does not inspect
transitive dependencies, runtime dlopen, foreign operating systems or SWF behavior.
"""
import json
from pathlib import Path
import platform
import re


BUNDLED_SONAME = re.compile(
    r'lib(?:flashvm|fvm_[A-Za-z0-9_]+|agg|cairo(?:-[A-Za-z0-9-]+)?|'
    r'freetype[0-9]*|fribidi|gif|harfbuzz(?:-[A-Za-z0-9-]+)?|'
    r'jpeg(?:-turbo)?|turbojpeg|png[0-9]*|nanosvg|unibreak|pixman(?:-[0-9]+)?|'
    r'sqlite[0-9]*|z|zlib)(?:-[0-9.]+)?\.so(?:\.[0-9.]+)?')


def elf_artifacts(document, build):
    """Select unique executable/shared ELF arguments inside the native build tree."""
    if (not isinstance(document, dict) or not isinstance(document.get('version'), dict)
            or document['version'].get('major') != 1
            or not isinstance(document.get('tests'), list)):
        raise ValueError('invalid CTest ELF discovery document')
    build = Path(build).resolve()
    artifacts = set()
    for test in document['tests']:
        command = test.get('command') if isinstance(test, dict) else None
        if (not isinstance(command, list) or not command
                or any(not isinstance(part, str) or not part for part in command)):
            raise ValueError('missing or malformed CTest ELF command')
        for part in command:
            path = Path(part)
            if not path.is_absolute():
                continue
            path = path.resolve()
            try:
                path.relative_to(build)
            except ValueError:
                continue
            if not path.is_file():
                continue
            with path.open('rb') as stream:
                header = stream.read(18)
            if (len(header) != 18 or header[:4] != b'\x7fELF'
                    or header[4] not in (1, 2) or header[5] not in (1, 2)
                    or header[6] != 1):
                continue
            kind = int.from_bytes(header[16:18], 'little' if header[5] == 1 else 'big')
            if kind in (2, 3):
                artifacts.add(path)
    if not artifacts:
        raise ValueError('no native CTest ELF executable artifacts discovered')
    return sorted(artifacts)


def needed_libraries(output):
    """Parse GNU readelf -d in C locale; reject malformed or unexpected evidence."""
    entries = []
    for line in output.splitlines():
        if '(NEEDED)' not in line:
            continue
        match = re.search(r'\(NEEDED\).*?\[([^\]\r\n]+)\]', line)
        if not match:
            raise ValueError('malformed ELF DT_NEEDED entry')
        entries.append(match[1])
    if '(NULL)' not in output:
        if output.strip() != 'There is no dynamic section in this file.':
            raise ValueError('missing ELF dynamic-section evidence')
    libraries = sorted(set(entries))
    forbidden = [name for name in libraries if BUNDLED_SONAME.fullmatch(Path(name).name)]
    if forbidden:
        raise ValueError('bundled libraries linked dynamically: ' + ', '.join(forbidden))
    return libraries


def validate_elf_dependencies(build, ctest, readelf, execute, record):
    """Record fail-first direct-dependency evidence; commands never execute an ELF."""
    evidence = {'status': 'failed', 'artifacts': [],
                'scope': 'direct DT_NEEDED of CTest ELF arguments; not transitive or dlopen'}
    record['elf_dependencies'] = evidence
    if platform.system() != 'Linux':
        raise RuntimeError('ELF dependency audit requires native Linux')
    execute('elf-tool', [readelf, '--version'])
    version = (build / 'elf-tool.log').read_text(encoding='utf-8').splitlines()
    if not version or not version[0].startswith('GNU readelf '):
        raise ValueError('GNU readelf version evidence is required')
    evidence['readelf_version'] = version[0]
    execute('elf-discover', [ctest, '--test-dir', str(build), '-C', 'Release',
                             '--show-only=json-v1'])
    artifacts = elf_artifacts(json.loads((build / 'elf-discover.log').read_text(encoding='utf-8')), build)
    for index, artifact in enumerate(artifacts):
        label = 'elf-dynamic-' + str(index).zfill(4)
        execute(label, [readelf, '--wide', '--dynamic', str(artifact)])
        libraries = needed_libraries((build / (label + '.log')).read_text(encoding='utf-8'))
        evidence['artifacts'].append({'path': str(artifact), 'needed': libraries})
    evidence['status'] = 'passed'
