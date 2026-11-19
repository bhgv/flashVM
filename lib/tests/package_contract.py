#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Verify release snapshots ignore dirty/generated files and reject external references.
# RU: Проверить независимость снимка от правок/генерируемых файлов и отказ внешних ссылок.
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]  # Used only to import the maintenance tool.
sys.path.insert(0, str(ROOT))
from lib.package import git, package, snapshot


# EN: Commit all fixture files under root with explicit local authorship; return commit.
# No remote connection or user configuration is touched.
# RU: Закоммитить fixture-файлы в root с явным авторством; вернуть commit.
# Без удалённых соединений и изменения пользовательской конфигурации.
def commit(root):
    git(root, 'add', '.')
    git(root, '-c', 'user.name=bhgv', '-c', 'user.email=bhgv@bhgv.bhgv', 'commit', '-qm', 'fixture')
    return git(root, 'rev-parse', 'HEAD').decode().strip()


# EN: Require snapshot rejection with the supplied text; root/revision are borrowed.
# Raise if a nonregular/reserved entry could reach a release archive.
# RU: Требовать отказ snapshot с текстом text; root/revision заимствованы.
# Сообщить ошибку, если необычная/зарезервированная запись попала бы в выпуск.
def refused(root, revision, text):
    try:
        snapshot(root, revision, 'styx')
    except ValueError as error:
        if text in str(error):
            return
        raise
    raise ValueError('snapshot accepted ' + text)


# EN: Own a disposable Git fixture; compare source bytes, modes and archive hashes
# before/after dirty files, local archive settings and export-ignore attributes.
# Also require rejection of a symlink and collision with generated RELEASE.json.
# RU: Владеть временным Git fixture; сравнить байты, режимы и хеши архивов до/после
# правок, локальных archive-настроек и export-ignore. Проверить отказ symlink
# и конфликта с генерируемым RELEASE.json.
def main():
    with tempfile.TemporaryDirectory(prefix='package-contract-', dir=ROOT.parent) as temporary:
        root = Path(temporary)
        git(root, 'init', '-q')
        source = root / 'lib/styx'
        source.mkdir(parents=True)
        (source / 'README.md').write_bytes(b'committed text\n')
        (source / 'probe.bin').write_bytes(b'\x00\xff\r\n')
        (source / 'run.sh').write_bytes(b'#!/bin/sh\nexit 0\n')
        (source / 'run.sh').chmod(0o755)
        (source / '.gitattributes').write_text('probe.bin export-ignore\n')
        revision = commit(root)
        expected = snapshot(root, revision, 'styx')
        if ('probe.bin', b'\x00\xff\r\n', 0o644) not in expected or ('run.sh', b'#!/bin/sh\nexit 0\n', 0o755) not in expected:
            raise ValueError('source bytes/modes changed by archive attributes')
        output = root / 'releases'
        output.mkdir()
        first = package(root, output, revision, 1234567890, 'styx')
        (source / 'README.md').write_bytes(b'dirty replacement\n')
        (source / 'build').mkdir()
        (source / 'build/generated.o').write_bytes(b'untracked object')
        git(root, 'config', 'tar.umask', '0777')
        second = package(root, output, revision, 1234567890, 'styx')
        if first != second or snapshot(root, revision, 'styx') != expected:
            raise ValueError('release depends on worktree/local archive settings')
        os.symlink('../outside', source / 'link')
        refused(root, commit(root), 'non-regular')
        (source / 'link').unlink()
        (source / 'RELEASE.json').write_text('{}\n')
        refused(root, commit(root), 'reserved')
    print('PASS release snapshot: exact blobs/modes, dirty isolation, attributes and rejected links/reserved paths')


if __name__ == '__main__':
    main()
