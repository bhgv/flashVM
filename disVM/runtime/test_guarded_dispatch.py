"""Audit the bounded generated x64 selector/actual C dispatch correspondence."""
from pathlib import Path
import argparse
import hashlib
import json
import re


class DispatchMismatch(ValueError):
    """A precise source-closure mismatch, never an execution-admission verdict."""


def inspect(emitter, dispatcher, executor):
    """Borrow three exact source texts; return names or refuse any bounded-shape drift.

    This checks the selected source closure, not arbitrary C semantics, machine
    code, ownership, error paths or parallel admission. No file is modified.
    """
    start = emitter.find('if(dvm_runtime_guarded_copies &&')
    end = emitter.find('return x64guardedcopy(out, cap, abi, op);',start)
    if start < 0 or end < 0 or emitter.count('if(dvm_runtime_guarded_copies &&') != 1:
        raise DispatchMismatch('selector shape')
    selected = re.findall(r'op == (I[A-Z0-9]+)',emitter[start:end])
    if not selected or len(selected) != len(set(selected)):
        raise DispatchMismatch('selector uniqueness')
    start = dispatcher.find('    switch (opcode)\n    {')
    end = dispatcher.find('    default:\n        return 0;',start)
    if start < 0 or end < 0 or dispatcher.count('    switch (opcode)\n    {') != 1:
        raise DispatchMismatch('dispatcher shape')
    body = dispatcher[start+len('    switch (opcode)\n    {'):end]
    cases = re.findall(r'\s*case (I[A-Z0-9]+):\s*([a-z][a-z0-9]*)\(\);\s*break;',body)
    residue = re.sub(r'\s*case I[A-Z0-9]+:\s*[a-z][a-z0-9]*\(\);\s*break;','',body)
    if residue.strip() or len(cases) != len({op for op,fn in cases}):
        raise DispatchMismatch('dispatcher exactly-once shape')
    if {op for op,fn in cases} != set(selected):
        raise DispatchMismatch('selector dispatch agreement')
    for op,fn in cases:
        entry = op.lower() if op in ("IEXPW", "IEXPL", "IEXPF") else op[1:].lower()
        if fn != entry:
            raise DispatchMismatch('opcode entry identity')
        if len(re.findall(r'\bOP\('+fn+r'\)',executor)) != 1:
            raise DispatchMismatch('executor identity')
    if not re.search(r'default:\s*return 0;\s*}\s*return 1;\s*}',dispatcher[end:]):
        raise DispatchMismatch('dispatcher completion')
    return sorted(cases)


def require_refusal(emitter, dispatcher, executor, expected):
    """A private omitted/misrouted case must refuse at its exact named predicate."""
    try:
        inspect(emitter,dispatcher,executor)
    except DispatchMismatch as failure:
        if str(failure) != expected:
            raise RuntimeError('wrong dispatch control stage: '+str(failure)) from failure
    else:
        raise RuntimeError('dispatch control missed: '+expected)


def main():
    """CLI supplies matching build/native and source runtime directories.

    --report writes a source-identity receipt; no VM permission follows. Each
    generated selector receives omission/misroute/false-completion controls.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path)
    parser.add_argument('--runtime',type=Path,default=Path(__file__).resolve().parent)
    parser.add_argument('--report',type=Path)
    args = parser.parse_args()
    texts = [(args.native/'amd64-emit.h').read_text(),(args.runtime/'bytes_fast.c').read_text(),
             (args.native/'xec.c').read_text()]
    emitter,dispatcher,executor = texts
    cases = inspect(*texts)
    controls = 0
    for op,fn in cases:
        before = '    case '+op+':\n        '+fn+'();\n        break;'
        if dispatcher.count(before) != 1:
            raise RuntimeError('dispatch control anchor drift: '+op)
        require_refusal(emitter,dispatcher.replace(before,'',1),executor,'selector dispatch agreement')
        require_refusal(emitter,dispatcher.replace(before,before.replace(fn+'();','wrong();'),1),executor,'opcode entry identity')
        require_refusal(emitter,dispatcher.replace(before,'    case '+op+':\n        return 1;',1),executor,'dispatcher exactly-once shape')
        controls += 3
    require_refusal(emitter.replace('op == '+cases[0][0],'op == '+cases[0][0]+' || op == '+cases[0][0],1),
                    dispatcher,executor,'selector uniqueness')
    controls += 1
    report = {'scope':'bounded generated-selector/source-dispatch/entry closure only; no execution authority',
              'operations':dict(cases),'operation_count':len(cases),'exact_source_controls':controls,
              'sha256':dict(zip(('amd64-emit.h','bytes_fast.c','xec.c'),
                               (hashlib.sha256(s.encode()).hexdigest() for s in texts)))}
    if args.report:
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS guarded dispatch closure: '+str(len(cases))+' operations; '+str(controls)+' exact source controls')


if __name__ == '__main__':
    main()
