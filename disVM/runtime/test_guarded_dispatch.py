"""Audit the bounded generated x64 selector/actual C dispatch correspondence."""
from pathlib import Path
import argparse
import hashlib
import json
import re


# These families use separate paths from the guarded byte dispatcher.
# Membership records where to audit, not an admission or safety result.
SEPARATE_PATHS = {
    'scheduler_and_calls': 'IALT INBALT ICALL IFRAME ISPAWN ILOAD IMCALL IMSPAWN IMFRAME IRET IEXIT ISEND IRECV IRAISE',
    'control_and_tables': 'INOP IGOTO IRUNT IJMP ICASE ICASEC ICASEL ITCMP IECLR',
    'allocation': 'INEW INEWA INEWCB INEWCW INEWCF INEWCP INEWCM INEWCMP INEWCL IMNEWZ INEWZ INEWAZ',
    'managed_owners': 'ICONSB ICONSW ICONSP ICONSF ICONSM ICONSMP ICONSL IHEADP IHEADMP ITAIL IMOVP IMOVMP ISELF',
    'strings_and_slices': 'ICVTCA ICVTAC ICVTWC ICVTCW ICVTFC ICVTCF IINSC IINDC IADDC ISLICEA ISLICELA ISLICEC ICVTLC ICVTCL',
}


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
    executor = re.sub(r'/\*.*?\*/|//[^\n]*', '', executor, flags=re.S)
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


def opcode_inventory(isa, optab, guarded):
    """Map every numeric opcode to its interpreter entry and audited x64 route."""
    def tokens(text, pattern):
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
        match = re.search(pattern, clean, flags=re.S)
        if not match:
            raise DispatchMismatch('opcode inventory shape')
        return [token.strip() for token in match.group(1).split(',') if token.strip()]

    names = tokens(isa, r'enum\s*\{([^}]*)\}')
    entries = tokens(optab, r'optab\[256\]\)\(void\)\s*=\s*\{([^}]*)\}')
    if (len(names) != len(entries) or not names or names[-1] != 'ISELF'
            or len(names) != len(set(names))
            or any(not re.fullmatch(r'I[A-Z0-9]+', name) for name in names)
            or any(not re.fullmatch(r'[a-z][a-z0-9]*', entry) for entry in entries)
            or not guarded.keys() <= set(names)):
        raise DispatchMismatch('opcode inventory coverage')

    remaining = [(name, family) for family, group in SEPARATE_PATHS.items() for name in group.split()]
    separate = dict(remaining)
    if len(separate) != len(remaining) or set(separate) != set(names) - guarded.keys():
        raise DispatchMismatch('separate opcode path coverage')

    rows = []
    for opcode, (name, entry) in enumerate(zip(names, entries)):
        guarded_entry = guarded.get(name)
        if guarded_entry is not None and guarded_entry != entry:
            raise DispatchMismatch('guarded x64/interpreter entry disagreement: '+name)
        rows.append({'opcode': opcode, 'name': name, 'interpreter_entry': entry,
                     'guarded_bytes_x64': guarded_entry is not None,
                     'audit_path': separate.get(name, 'guarded_bytes_x64')})
    return rows


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
    isa = (args.native/'isa.h').read_text()
    optab = (args.native/'optab.h').read_text()
    inventory = opcode_inventory(isa, optab, dict(cases))
    first_guarded = next(row for row in inventory if row['guarded_bytes_x64'])
    anchor = '\t'+first_guarded['interpreter_entry']+','
    try:
        opcode_inventory(isa, optab.replace(anchor, '\twrong,', 1), dict(cases))
    except DispatchMismatch as failure:
        if str(failure) != 'guarded x64/interpreter entry disagreement: '+first_guarded['name']:
            raise
    else:
        raise RuntimeError('interpreter mismatch control missed')
    controls = 1
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
              'opcode_inventory':inventory, 'operations':dict(cases),'operation_count':len(cases),'exact_source_controls':controls,
              'sha256':dict(zip(('amd64-emit.h','bytes_fast.c','xec.c','isa.h','optab.h'),
                               (hashlib.sha256(s.encode()).hexdigest() for s in [*texts, isa, optab])))}
    if args.report:
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS guarded dispatch closure: {len(cases)} of {len(inventory)} opcodes; {controls} exact source controls')


if __name__ == '__main__':
    main()
