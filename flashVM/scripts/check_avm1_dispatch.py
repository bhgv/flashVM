#!/usr/bin/env python3
"""Audit a pinned opcode catalog against named constants and interpreter references.

This is a source inventory, not a semantic conformance checker. JSON goes to
stdout; generated reports should be redirected outside the Git source tree.
"""
import argparse
import csv
import json
import re
from pathlib import Path

REPORT_SCHEMA_VERSION = 1
MAX_ACTION_BYTE = 0xFF
CATALOG_REFERENCE = (
    "https://github.com/ruffle-rs/ruffle/blob/"
    "2372a4e148a67d68efd796f4477b3cddd4f880cd/swf/src/avm1/opcode.rs"
)
ACTION_DEFINE = re.compile(
    r"^\s*#define\s+(FVM_AVM1_ACTION_\w+)\s+"
    r"(0[xX][0-9a-fA-F]+|[0-9]+)[uU]?\s*$", re.MULTILINE
)
C_NONCODE = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
    re.DOTALL,
)


def code_text(text):
    """Exclude comments and quoted literals from the conservative reference audit."""
    return C_NONCODE.sub(" ", text)


def load_catalog(path):
    """Read factual byte/name pairs and reject malformed or duplicate catalog entries."""
    with path.open(newline="", encoding="utf-8") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != ["opcode", "name"]:
            raise ValueError("catalog columns must be opcode,name")
        rows = []
        codes, names = set(), set()
        for row in reader:
            if None in row:
                raise ValueError("extra catalog columns in a row")
            opcode = int(row["opcode"], 16)
            name = row["name"]
            if not 0 <= opcode <= MAX_ACTION_BYTE or not name:
                raise ValueError("invalid catalog byte/name")
            if opcode in codes or name in names:
                raise ValueError("duplicate catalog byte/name")
            codes.add(opcode)
            names.add(name)
            rows.append((opcode, name))
        if not rows:
            raise ValueError("empty opcode catalog")
        return sorted(rows)


def audit(root, catalog):
    """Report macro presence and execution-loop references without claiming semantics."""
    header = code_text((root / "src/avm1/action_codes.h").read_text(encoding="utf-8"))
    macros = {}
    names = set()
    for macro, value in ACTION_DEFINE.findall(header):
        opcode = int(value, 0)
        if not 0 <= opcode <= MAX_ACTION_BYTE:
            raise ValueError("Action macro outside byte range: " + value)
        if opcode in macros or macro in names:
            raise ValueError("duplicate Action macro byte/name: " + macro)
        macros[opcode] = macro
        names.add(macro)
    source = code_text((root / "src/avm1/interpreter.cpp").read_text(encoding="utf-8"))
    start = source.find("while (pc < actions.size())")
    end = source.find("FVM_Result fvm_avm1_execute(", start)
    if start < 0 or end < 0:
        raise ValueError("cannot locate interpreter execution-loop boundaries")
    references = set(re.findall(r"\bFVM_AVM1_ACTION_\w+\b", source[start:end]))
    entries = []
    for opcode, name in load_catalog(catalog):
        macro = macros.get(opcode)
        entries.append({
            "opcode": f"0x{opcode:02X}", "name": name, "macro": macro,
            "dispatch_present": macro is not None and macro in references,
        })
    known = {int(row["opcode"], 16) for row in entries}
    return {
        "schema": REPORT_SCHEMA_VERSION,
        "catalog_reference": CATALOG_REFERENCE,
        "scope": "source references only; no semantic, version or player certification",
        "catalog_count": len(entries),
        "dispatch_present_count": sum(row["dispatch_present"] for row in entries),
        "dispatch_absent": [row["name"] for row in entries if not row["dispatch_present"]],
        "outside_catalog": [f"0x{byte:02X}" for byte in sorted(set(macros) - known)],
        "entries": entries,
    }


def main():
    """Print the inventory and optionally fail a gate when catalog dispatches are missing."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--catalog", type=Path)
    parser.add_argument("--require-all", action="store_true",
                        help="return failure if any catalog entry lacks a dispatch reference")
    args = parser.parse_args()
    try:
        report = audit(args.root, args.catalog or args.root / "doc/AVM1_DISPATCH_CATALOG.csv")
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.error(str(error))
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return int(args.require_all and bool(report["dispatch_absent"]))


if __name__ == "__main__":
    raise SystemExit(main())
