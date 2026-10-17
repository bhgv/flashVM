#!/usr/bin/env python3
"""Source-table order must be stable when macro/function names differ only in case."""
import os
import pathlib
import subprocess
import sys
import tempfile


def child(path):
    import link_documentation_sources as links
    # Independent declaration-index fixture: both real symbol spellings share a header.
    index = {
        'FVM_SWF_SOUND_EVENT_ENVELOPE': (0, 'include/flashvm/swf_sound.h', 1, 'macro / define'),
        'fvm_swf_sound_event_envelope': (0, 'include/flashvm/swf_sound.h', 2, 'function'),
    }
    _, rendered = links.refresh(path, index)
    path.write_text(rendered, encoding='utf-8')
    before, after = links.refresh(path, index)
    assert before == after, 'Source-link refresh is not idempotent'
    sys.stdout.write(rendered)


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--child':
        child(pathlib.Path(sys.argv[2]))
        return 0
    results = []
    # Keep fixture and declared source targets on the same Windows drive.
    base = pathlib.Path(__file__).resolve().parents[1]/'build-documentation-links'
    base.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=base, prefix='flashvm-doc-order-') as directory:
        for seed in ('0', '1', '2', '3', '42', '77'):
            path = pathlib.Path(directory)/('seed-'+seed+'.md')
            path.write_text('`fvm_swf_sound_event_envelope` and `FVM_SWF_SOUND_EVENT_ENVELOPE`.\n', encoding='utf-8')
            env = dict(os.environ, PYTHONHASHSEED=seed)
            result = subprocess.run([sys.executable, __file__, '--child', str(path)],
                                    env=env, capture_output=True, text=True, check=True)
            results.append(result.stdout)
    assert len(set(results)) == 1, 'Casefold ties depend on randomized set traversal'
    text = results[0]
    assert text.index('| `FVM_SWF_SOUND_EVENT_ENVELOPE`') < text.index('| `fvm_swf_sound_event_envelope`')
    print('PASS deterministic source links across six hash seeds and repeated refresh')
    return 0


if __name__ == '__main__':
    sys.exit(main())
