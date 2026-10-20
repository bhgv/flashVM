"""Unicode/escape lexical checks, independent of executable FIR support."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unicodedata

exe = sys.argv[1]
good = [
    r'var \u0061=1; \u0061++;',
    r'var a\u0301=1; var \u03b1=2;',
    'var Редактор=1; var имя=2; var 𐐀=3;',
    'var a\u200cb=1; var a\u200db=2;',
    'var\u00a0name=1; var\u1680other=2;',
    'var x="\u00a0\u2028";', # Replaced below: raw LS in strings is tested separately.
    'function f(){return\u2028value}',
    'var x=1\u2029var y=2',
    '\ufeffvar x=1;\u2000var y=2;',
    'var\vname=1; var\fname=2;',
]
# Whitespace inside quoted strings is data; raw line terminators are not.
good[5] = 'var x="\u00a0";'
bad = [
    r'var \u0030name=1;', r'var \u0301name=1;', r'var \uD800=1;',
    r'var \uDC00=1;', r'var \u12=1;', r'var \u00GG=1;',
    r'var \u0069f=1;', r'\u0069f(true) {}',
    'var 😀=1;', 'var \u0301name=1;', 'var \u0660name=1;',
    'var name\u00d7tail=1;', 'function f(){throw\u2028value}',
]
good += [
    'var x="a\\\nb";', 'var x="a\\\r\nb";', 'var x="a\\\rb";',
    'var x="a\\\u2028b";', r'var x="\uD800\x41";',
    '// comment\u2028var x=1;', '/* block\u2029comment */ var x=1;',
]
bad += ['// comment\u2028var x=;', 'var x="raw\u2028line";',
        r'var x="\u12zz";', r'var x="\xZ0";']
good += ['label\u00a0: while(true) break label;',
         'label // c\u2028: while(true) break label;']
# Exercise boundaries of both category sets with the same pinned Unicode database.
# One source tests hundreds of accepted names without one process per character.
db = unicodedata.ucd_3_2_0
names = []
for code in range(128, 0x110000, 199):
    if db.category(chr(code)) in {'Lu','Ll','Lt','Lm','Lo','Nl'}:
        names.append('var '+chr(code)+'name=1;')
good.append('\n'.join(names))
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder)/'unicode.as'
    for accepted, cases in [(True,good),(False,bad)]:
        for source in cases:
            path.write_text(source, encoding='utf-8')
            run = subprocess.run([exe,str(path)],capture_output=True,timeout=5)
            assert run.returncode == (0 if accepted else 1), (source,run.returncode,run.stderr)
    path.write_text('var a=1;\u2028var \\u0030name=2;',encoding='utf-8')
    run = subprocess.run([exe,str(path)],capture_output=True,timeout=5)
    assert run.returncode == 1 and b':2:' in run.stderr, run.stderr
print(len(good)+len(bad)+1, 'Unicode cases,',len(names),'sampled names; syntax only')
