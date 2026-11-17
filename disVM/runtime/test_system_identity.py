# Copyright (c) 2026 bhgv
# EN: Persistent canonical installation identity, concurrent creation and nickname-only updates.
# RU: Постоянный canonical ID установки, concurrent creation и nickname-only updates.
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
import tempfile


# EN/RU: Run the actual provisioner at path; return copied JSON or raise on CLI failure.
# Выполнить настоящий provisioner для path; вернуть JSON либо raise при CLI failure.
def run(tool, path, *options):
    result = subprocess.run([sys.executable, tool, '--file', path, *options], capture_output=True, text=True, timeout=15)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout)


# EN/RU: Exercise actual file creation/read/update; bad existing content must never be regenerated silently.
# Проверить create/read/update; существующее неверное содержимое не перегенерируется молча.
def main():
    tool = Path(__file__).resolve().parents[2]/'tools/disvm-system-id.py'
    with tempfile.TemporaryDirectory(prefix='system identity ') as name:
        root = Path(name)
        path = root/'system.json'
        with ThreadPoolExecutor(max_workers=8) as workers:
            results = list(workers.map(lambda _: run(tool, path), range(16)))
        identity = results[0]['systemid']
        if len(identity) != 43 or any(x['systemid'] != identity for x in results):
            raise RuntimeError('concurrent creation changed installation identity')
        changed = run(tool, path, '--nickname', 'Лисичка')
        if changed['systemid'] != identity or changed['nickname'] != 'Лисичка' or run(tool, path) != changed:
            raise RuntimeError('nickname edit changed identity or did not persist')
        if run(tool, root/'second.json')['systemid'] == identity:
            raise RuntimeError('independent installations share identity')
        malformed = root/'bad.json'
        malformed.write_text('{broken')
        result = subprocess.run([sys.executable, tool, '--file', malformed], capture_output=True, text=True, timeout=15)
        if not result.returncode or malformed.read_text() != '{broken':
            raise RuntimeError('invalid existing identity was replaced')
        print('PASS persistent system identity concurrent creation nickname and refusal')


if __name__ == '__main__':
    main()
