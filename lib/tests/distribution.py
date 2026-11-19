#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Verify deterministic release contents and build extracted packages outside the repository.
# RU: Проверить детерминированность выпусков и сборку извлечённых пакетов вне репозитория.
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]  # Test harness only; never a package include path.
sys.path.insert(0, str(ROOT))
from lib.package import PACKAGES, git, package
from sdk import consumers


# EN: Require a test condition or report message; no side effects or ownership transfer.
# RU: Проверить условие либо сообщить message; без побочных эффектов/передачи владения.
def require(condition, message):
    if not condition:
        raise ValueError(message)


# EN: Validate archive receipt and every source hash/mode before writing beneath destination.
# Reject duplicates, nonregular records and unexpected paths. Return extracted package root.
# Archive paths are treated as data; no tar extraction command or external symlink is followed.
# RU: До записи в destination проверить квитанцию, хеш и режим каждого исходника.
# Отклонить повторы, необычные записи и пути. Вернуть корень извлечённого пакета.
# Пути архива — данные; команды tar extraction и внешние symlink не используются.
def unpack(archive_path, receipt, destination, commit):
    require(hashlib.sha256(archive_path.read_bytes()).hexdigest() == receipt['sha256'], 'archive checksum')
    name = receipt['package']
    records = {}
    modes = {}
    with tarfile.open(archive_path, 'r:gz') as archive:
        for entry in archive:
            require(entry.isfile(), 'nonregular release member')
            path = entry.name
            require(path.startswith(name + '/') and
                    all(part not in ('', '.', '..') for part in path.split('/')), 'invalid release path')
            require(path not in records, 'duplicate release member')
            records[path] = archive.extractfile(entry).read()
            modes[path] = entry.mode
    manifest = json.loads(records.pop(name + '/RELEASE.json'))
    require(manifest['format'] == 1 and manifest['commit'] == commit and manifest['package'] == name,
            'release identity mismatch')
    inventory = manifest['files']
    require(len(inventory) == receipt['source_files'], 'inventory count mismatch')
    require(len({item['path'] for item in inventory}) == len(inventory), 'duplicate manifest path')
    require(set(records) == {name + '/' + item['path'] for item in inventory}, 'unlisted/missing source')
    for item in inventory:
        path = name + '/' + item['path']
        require(hashlib.sha256(records[path]).hexdigest() == item['sha256'], 'source checksum: ' + path)
        require(oct(modes[path]) == item['mode'], 'source mode: ' + path)
    # Source/package completeness is checked before a compiler can see the directory.
    for path in ('README.md', 'README.ru.md', 'CMakeLists.txt', 'SOURCES.jsonl', 'view_docs.py', 'LICENSE.bhgv'):
        require(name + '/' + path in records, 'missing required source distribution file: ' + path)
    if name == 'styx':
        require(name + '/vendor/inferno/NOTICE' in records, 'missing Inferno notice')
        require(any(path.startswith(name + '/vendor/inferno/lib/legal/') for path in records),
                'missing Inferno license texts')
    records[name + '/RELEASE.json'] = (json.dumps(manifest, sort_keys=True, indent=2) + '\n').encode()
    for path, data in records.items():
        target = destination / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        target.chmod(modes[path])
    return destination / name


# EN: Run argv in borrowed cwd with private temporary-directory environment; append diagnostics.
# Raise on failed tools, preserving the combined acceptance log after temp cleanup.
# RU: Выполнить argv в cwd с личным TMPDIR; добавить диагностику в log.
# Передать отказ инструмента, сохранив общий лог после удаления temp-каталога.
def run(argv, cwd, log):
    with log.open('a') as output:
        output.write('\n$ ' + ' '.join(map(str, argv)) + '\n')
        output.flush()
        subprocess.run(list(map(str, argv)), cwd=cwd, env=dict(os.environ, TMPDIR=str(cwd)),
                       stdout=output, stderr=subprocess.STDOUT, check=True)


# EN: Own a temporary workspace outside ROOT; verify two releases byte-for-byte,
# build both profiles, move installed SDKs, run consumers/docs and wire integration.
# CLI supplies cmake and optional Git revision; logs remain in ROOT/build.
# RU: Владеть временным каталогом вне ROOT; сравнить два выпуска побайтно,
# собрать оба профиля, перенести SDK, выполнить consumers/docs и wire-интеграцию.
# CLI задаёт cmake и необязательный Git revision; лог остаётся в ROOT/build.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cmake', type=Path)
    parser.add_argument('--revision', default='HEAD')
    args = parser.parse_args()
    cmake = args.cmake.resolve()
    commit = git(ROOT, 'rev-parse', '--verify', '--end-of-options', args.revision + '^{commit}').decode().strip()
    epoch = int(git(ROOT, 'show', '-s', '--format=%ct', commit))
    log = ROOT / 'build/library-distribution-acceptance.log'
    log.parent.mkdir(exist_ok=True)
    log.write_text('Source revision: ' + commit + '\n')
    with tempfile.TemporaryDirectory(prefix='library-release-', dir=ROOT.parent) as temporary:
        area = Path(temporary)
        releases = [area / 'first', area / 'second']
        for output in releases:
            output.mkdir()
        for name, option in zip(PACKAGES, ('STYX_HOSTED', 'UNIBUS_QUEUES')):
            receipts = [package(ROOT, output, commit, epoch, name) for output in releases]
            require(receipts[0] == receipts[1], 'nonreproducible receipt: ' + name)
            require((releases[0] / receipts[0]['file']).read_bytes() ==
                    (releases[1] / receipts[1]['file']).read_bytes(), 'nonreproducible gzip: ' + name)
            source = unpack(releases[0] / receipts[0]['file'], receipts[0], area / 'source', commit)
            for hosted in (True, False):
                label = name + ('-hosted' if hosted else '-minimal')
                build = area / label
                run([cmake, '-S', source, '-B', build, '-DBUILD_TESTING=ON',
                     '-D' + option + '=' + ('ON' if hosted else 'OFF'),
                     '-DCMAKE_INSTALL_INCLUDEDIR=api/include', '-DCMAKE_CXX_COMPILER=/unavailable-cxx'], area, log)
                run([cmake, '--build', build, '-j4'], area, log)
                run([cmake.with_name('ctest'), '--test-dir', build, '--output-on-failure'], area, log)
                installed = area / (label + '-installed')
                run([cmake, '--install', build, '--prefix', installed], area, log)
                relocated = area / (label + '-relocated')
                installed.rename(relocated)
                consumers(cmake, relocated, area / (label + '-consumer'), name, hosted, log)
                run([sys.executable, ROOT / 'lib/tests/documentation.py', relocated / 'share' / name], area, log)
                print('PASS archive, external build, relocated SDK and docs:', label, flush=True)
        run([sys.executable, ROOT / 'lib/tests/interoperability.py', area / 'styx-hosted',
             area / 'libunibus-hosted'], area, log)
        print('PASS source distributions: reproducible bytes and five cross-library models', flush=True)


if __name__ == '__main__':
    main()
