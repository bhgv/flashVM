#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Publish deterministic, separate source archives from an explicit Git snapshot.
# RU: Подготовить детерминированные отдельные архивы исходников из снимка Git.
from pathlib import Path
import argparse
import gzip
import hashlib
import io
import json
import subprocess
import tarfile

PACKAGES = ('styx', 'libunibus')  # Separate projects, never a combined runtime dependency.


# EN: Read Git output as bytes in borrowed root; argv never passes through a shell.
# Return owned bytes or propagate a Git failure without producing a partial archive.
# RU: Прочитать вывод Git в root; argv не проходит через shell. Вернуть байты
# либо передать ошибку Git без публикации неполного архива.
def git(root, *arguments):
    return subprocess.check_output(['git', '-C', str(root), *arguments])


# EN: Return sorted regular files (name, bytes, mode) for package at commit.
# Read only committed Git objects, preserving notices and exact binary contents.
# Refuse links/special entries instead of depending on an external filesystem.
# RU: Вернуть отсортированные обычные файлы (имя, байты, режим) package в commit.
# Читать лишь Git-объекты, сохраняя notices и байты. Отклонять ссылки/спецфайлы,
# чтобы архив не зависел от внешней файловой системы.
def snapshot(root, commit, package):
    prefix = 'lib/' + package + '/'
    entries = []
    for record in git(root, 'ls-tree', '-rz', '--full-tree', commit, '--', 'lib/' + package).split(b'\0'):
        if not record:
            continue
        metadata, path = record.split(b'\t', 1)
        mode, kind, oid = metadata.split()
        path = path.decode('utf-8')
        if kind != b'blob' or mode not in (b'100644', b'100755') or not path.startswith(prefix):
            raise ValueError('non-regular package member: ' + path)
        name = path[len(prefix):]
        if name == 'RELEASE.json' or any(part in ('', '.', '..') for part in name.split('/')):
            raise ValueError('invalid/reserved package path: ' + name)
        entries.append((name, oid, 0o755 if mode == b'100755' else 0o644))
    if not entries:
        raise ValueError('empty package: ' + package)
    # Batch exact Git objects: archive filters, dirty files and local tar options cannot alter source bytes.
    data = subprocess.check_output(['git', '-C', str(root), 'cat-file', '--batch'],
                                   input=b''.join(oid + b'\n' for _, oid, _ in entries))
    cursor = 0
    files = []
    for name, oid, mode in entries:
        end = data.index(b'\n', cursor)
        actual, kind, size = data[cursor:end].split()
        count = int(size)
        cursor = end + 1
        if actual != oid or kind != b'blob' or data[cursor + count:cursor + count + 1] != b'\n':
            raise ValueError('invalid Git batch response: ' + name)
        files.append((name, data[cursor:cursor + count], mode))
        cursor += count + 1
    if cursor != len(data):
        raise ValueError('trailing Git batch data')
    return sorted(files)


# EN: Add one owned byte payload under name/mode to archive with epoch metadata.
# No extraction or host path access; tar owns a copy while the call runs.
# RU: Добавить байты под name/mode в archive с метаданными epoch.
# Без извлечения и доступа к host-пути; tar получает копию на время вызова.
def add(archive, name, data, mode, epoch):
    entry = tarfile.TarInfo(name)
    entry.size = len(data)
    entry.mode = mode
    entry.mtime = epoch
    archive.addfile(entry, io.BytesIO(data))


# EN: Write package archive under output from commit/epoch; return its receipt.
# Normalize metadata and gzip header; source bytes and executable bits stay exact.
# Replace the destination only after successful closure of the temporary file.
# RU: Записать архив package в output из commit/epoch; вернуть квитанцию.
# Нормализовать метаданные/gzip, сохранив байты и исполняемые биты. Подменить
# назначение только после успешного закрытия временного файла.
def package(root, output, commit, epoch, name):
    files = snapshot(root, commit, name)
    inventory = [{'path': path, 'mode': oct(mode), 'sha256': hashlib.sha256(data).hexdigest()}
                 for path, data, mode in files]
    manifest = {'format': 1, 'package': name, 'commit': commit, 'files': inventory}
    filename = name + '-' + commit[:12] + '.tar.gz'
    destination = output / filename
    temporary = destination.with_suffix('.tmp')
    try:
        with temporary.open('wb') as stream:
            with gzip.GzipFile(filename='', fileobj=stream, mode='wb', mtime=epoch) as compressed:
                with tarfile.open(fileobj=compressed, mode='w|', format=tarfile.USTAR_FORMAT) as archive:
                    for path, data, mode in files:
                        add(archive, name + '/' + path, data, mode, epoch)
                    payload = (json.dumps(manifest, ensure_ascii=False, sort_keys=True, indent=2) + '\n').encode()
                    add(archive, name + '/RELEASE.json', payload, 0o644, epoch)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    return {'file': filename, 'sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
            'source_files': len(files), 'package': name}


# EN: CLI revision defaults to HEAD; output is an explicit archive directory.
# Resolve the commit once so both packages and receipts share one snapshot.
# Return normally on success; argument, Git and filesystem failures propagate.
# RU: CLI revision по умолчанию HEAD; output — явный каталог архивов.
# Разрешить commit один раз для общего снимка двух пакетов и квитанций.
# Успех — обычный возврат; ошибки аргументов, Git и файлов передаются.
def main():
    parser = argparse.ArgumentParser(description="Create separate reproducible Styx/libunibus source archives")
    parser.add_argument('--revision', default='HEAD', help='committed Git revision (default: HEAD)')
    parser.add_argument('--output', type=Path, required=True, help='destination directory')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    commit = git(root, 'rev-parse', '--verify', '--end-of-options', args.revision + '^{commit}').decode().strip()
    epoch = int(git(root, 'show', '-s', '--format=%ct', commit))
    args.output.mkdir(parents=True, exist_ok=True)
    receipts = [package(root, args.output, commit, epoch, name) for name in PACKAGES]
    checksums = ''.join(item['sha256'] + '  ' + item['file'] + '\n' for item in receipts)
    (args.output / 'SHA256SUMS').write_text(checksums)
    print(checksums, end='')


if __name__ == '__main__':
    main()
