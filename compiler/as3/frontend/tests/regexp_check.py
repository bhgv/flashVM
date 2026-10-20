"""Pattern syntax is tested separately from runtime matching and FIR lowering."""
from pathlib import Path
import subprocess
import sys
import tempfile

exe = sys.argv[1]
good = [
    r'a|b', r'(ab|cd)+', r'()', r'a|', r'|a', r'[]', r'[^]',
    r'[a-zA-Z0-9_]+', r'[\x41-\u005A]', r'[-a]', r'[a-]', r'[\d\s]+',
    r'a{1}', r'a{1,}', r'a{1,5}?', r'a{0001,02}+',
    r'(?:a)(?=b)b', r'a(?!c)', r'(?<=a)b', r'(?<!a)b', r'(?>a|ab)b',
    r'(?P<name>\d+)\k<name>', r'(?P<name>a)(?P=name)', r'\1(a)', r'(a)\1',
    r'\cA\t\n\r\v\f\x41\uD800', r'[\b\012]', r'^a$|\bword\B',
]
bad = [
    r'(', r')', r'(a', r'a)', r'(?q:a)', r'(?P<>a)', r'(?P<1bad>a)',
    r'(?P<n>a)(?P<n>b)', r'\k<missing>', r'(?P=missing)', r'\1',
    r'[a', r'[z-a]', r'[\d-a]', r'[a-\w]', r'[\xZZ]', r'\u12QQ', r'\c1',
    r'*a', r'a**', r'a+*', r'a???', r'^+', r'\b*', r'(?=a)+',
    r'a{,3}', r'a{3,2}', r'a{99999999999999999999,2}', r'a{2', r'a{2,,3}',
]
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder)/'pattern.as'
    for accepted, patterns in [(True,good),(False,bad)]:
        for pattern in patterns:
            path.write_text('var r=/'+pattern+'/;',encoding='utf-8')
            run = subprocess.run([exe,str(path)],capture_output=True,timeout=5)
            assert run.returncode == (0 if accepted else 1), (pattern,run.returncode,run.stderr)
            if not accepted and not pattern.startswith('*'):
                assert b'E1103' in run.stderr and b'byte 6' in run.stderr, (pattern,run.stderr)
    for source, accepted in [('var r=/a b # ignored/x;',True),
                             ('var r=/a{ 1}/x;',False),
                             ('var r=/a/\u00a0; var x=1;',True),
                             ('var r=/a\\\u2028b/;',False)]:
        path.write_text(source,encoding='utf-8')
        run = subprocess.run([exe,str(path)],capture_output=True,timeout=5)
        assert run.returncode == (0 if accepted else 1), (source,run.returncode,run.stderr)
    # Every truncated group/escape/class prefix terminates, including malformed flags.
    seed = 'var r=/(?P<name>[a-z]{2,4})\\k<name>/gi;'
    for end in range(len(seed)+1):
        path.write_text(seed[:end],encoding='utf-8')
        run = subprocess.run([exe,str(path)],capture_output=True,timeout=5)
        assert run.returncode in (0,1), (seed[:end],run.returncode,run.stderr)
print(len(good)+len(bad)+4,'regexp acceptance cases plus',len(seed)+1,'truncations; no matching claim')
