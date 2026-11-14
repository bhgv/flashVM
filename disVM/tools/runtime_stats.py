#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Normalize existing native diagnostic reports without losing counters or integer precision.
# RU: Нормализовать native diagnostic reports без потери counters и integer precision.
import argparse
import json
from pathlib import Path
import re
import sys

TAG = re.compile(r"(?:NATIVE|PROXY|MANAGED)[A-Z0-9_]*\Z")  # Extensible diagnostic families.
KEY = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")  # Stable field names, not expressions.


# EN: Borrow text, return independent records/errors. Preserve field values as strings
# so uint64 counters survive JSON consumers. Invalid records publish no partial fields.
# No target execution, pointer dereference or coherent live snapshot is implied.
# RU: Borrow text, вернуть independent records/errors. Значения остаются strings,
# чтобы uint64 counters сохранялись в JSON consumers. Invalid records не публикуют
# partial fields. Target execution, pointer dereference и coherent live snapshot не выполняются.
def parse(text):
    records = []
    errors = []
    for number, line in enumerate(text.splitlines(), 1):
        tokens = line.split()
        if not tokens:
            continue
        if tokens[0].startswith("instructions="):
            tag, body = "RUNTIME_TOTALS", tokens
        elif TAG.fullmatch(tokens[0]):
            tag, body = tokens[0], tokens[1:]
        else:
            continue
        fields = {}
        labels = []
        problem = None
        for token in body:
            if "=" not in token:
                labels.append(token)
                continue
            key, value = token.split("=", 1)
            if not KEY.fullmatch(key) or not value or key in fields:
                problem = "invalid or duplicate field: " + key
                break
            fields[key] = value
        if problem or not fields:
            errors.append({"line": number, "tag": tag, "error": problem or "missing fields"})
        else:
            records.append({"line": number, "tag": tag, "labels": labels, "fields": fields})
    return {
        "schema": "disvm-runtime-observation-v1",
        "source": "diagnostic-report",
        "coherent_live_snapshot": False,
        "complete": not errors,
        "records": records,
        "errors": errors,
    }


# EN/RU: Read UTF-8 report path or stdin; emit JSON and fail explicitly on malformed diagnostic records.
# Читать UTF-8 report path либо stdin; выдать JSON и explicit failure для malformed diagnostic records.
def main():
    parser = argparse.ArgumentParser(description="Normalize DisVM native diagnostic reports")
    parser.add_argument("report", nargs="?", help="UTF-8 report; omit to read stdin")
    args = parser.parse_args()
    text = Path(args.report).read_text(encoding="utf-8") if args.report else sys.stdin.read()
    result = parse(text)
    json.dump(result, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if result["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
