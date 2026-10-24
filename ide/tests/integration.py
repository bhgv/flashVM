#!/usr/bin/env python3
"""Acceptance against explicitly selected real compiler/linker/FlashVM tools."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    """Exercise all four numeric profiles through the public IDE CLI protocol."""
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['compiler', 'ir', 'linker', 'runner']:
        parser.add_argument('--'+name, required=True, type=lambda p: str(Path(p).resolve()))
    args = parser.parse_args()
    driver = Path(__file__).resolve().parents[1] / 'build.py'
    cases = {
        'as1': ('trace(6*7);\n', '42\n'),
        'as2': ('var x:Number=6; x*=7; trace(x);\n', '42\n'),
        'as3': ('const x:Number=6; trace(x*7);\n', '42\n'),
        'limbo': ('implement Answer; Answer: module { value: fn(): int; }; value(): int { return 6*7; }\n', '42\n'),
    }
    checks = 0
    with tempfile.TemporaryDirectory(prefix='ide-real-') as directory:
        root = Path(directory)
        source, output = root/'source with spaces.as', root/'movie with spaces.swf'
        for language, (text, expected) in cases.items():
            command = [sys.executable, str(driver), str(source), str(output), '--language', language,
                       '--compiler', args.compiler, '--ir', args.ir, '--linker', args.linker,
                       '--runner', args.runner, '--module', 'ide.'+language]
            for optimise in (True, False):
                source.write_text(text, encoding='utf-8')
                request = command + ['--expect-trace', expected] + ([] if optimise else ['--no-optimise'])
                result = subprocess.run(request, capture_output=True, timeout=60)
                report = json.loads(result.stdout)
                assert result.returncode == 0 and report['ok'] and report['published'], report
                assert output.read_bytes()[:4] == b'FWS\x08'
                checks += 1
            previous = output.read_bytes()
            source.write_text('@@@\n', encoding='utf-8')
            result = subprocess.run(command, capture_output=True, timeout=60)
            report = json.loads(result.stdout)
            assert result.returncode == 1 and not report['published'], report
            assert report['diagnostics'][0]['stage'] == 'source', report
            assert output.read_bytes() == previous
            checks += 1
            source.write_text(text, encoding='utf-8')
            result = subprocess.run(command+['--expect-trace', 'incorrect trace\n'], capture_output=True, timeout=60)
            report = json.loads(result.stdout)
            assert result.returncode == 1 and report['diagnostics'][0]['stage'] == 'run', report
            assert output.read_bytes() == previous
            assert not list(root.glob('.ide-build-*'))
            checks += 1
    print(json.dumps({'passed': checks, 'profiles': list(cases)}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
