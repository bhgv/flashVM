#!/usr/bin/env python3
"""Apply the native readability profile only to Git-tracked, handwritten files.

Use --write to edit files; the default reports files that would change.
The token guard permits added/relocated braces and whitespace/comment layout,
but rejects changes to every other native token, including string literals.
It is a layout check, not a substitute for compiler/runtime validation.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]  # Repository containing the owned profiles.
EXTENSIONS = {".c", ".cpp", ".cc", ".h", ".hpp", ".mm"}  # Native source/header suffixes.
TOKEN = re.compile(
    r"/\*.*?\*/|//[^\n]*|(?:L|u8|u|U)?\"(?:\\.|[^\"\\])*\""
    r"|(?:L|u|U)?'(?:\\.|[^'\\])*'|[A-Za-z_][A-Za-z_0-9]*"
    r"|[0-9]+(?:\.[0-9]+)?|[^\s]",
    re.DOTALL,
)


# Compare executable token spelling without layout, comments or inserted braces.
# Parameters: source is complete native text. Returns a list including literal tokens.
def native_tokens(source):
    """Return non-layout tokens; preserve literal contents and operator order."""
    result = []
    for match in TOKEN.finditer(source):
        token = match.group()
        if token.startswith(("/*", "//")) or token in {"{", "}", "\\"}:
            continue
        result.append(token)
    return result


# Keep macro/directive boundaries significant when checking formatting.
# Parameters: source is complete native text. Returns one token tuple per directive.
def directive_tokens(source):
    """Return directive bodies, retaining macro boundaries across continued lines.

    Parameters: source is complete native text. Returns one token tuple per
    preprocessor directive. Losing a continuation must never pass as whitespace.
    """
    result = []
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        if not lines[index].lstrip().startswith("#"):
            index += 1
            continue
        pieces = [lines[index]]
        while pieces[-1].rstrip().endswith("\\") and index + 1 < len(lines):
            index += 1
            pieces.append(lines[index])
        result.append(tuple(native_tokens("\n".join(pieces))))
        index += 1
    return result


# Add declaration/include spacing without editing native expressions or macro bodies.
# Parameters: source is Allman-formatted native text. Returns text with a final newline.
def declaration_spacing(source):
    """Reserve two blank lines before definition comments and declaration blocks.

    Clang-format establishes Allman braces first. This pass only inserts empty
    lines, leaving native tokens and preprocessor directives in place.
    """
    lines = source.splitlines()
    targets = set()
    protected = set()
    continuation = False
    for index, line in enumerate(lines):
        directive = continuation or line.lstrip().startswith("#")
        if directive:
            protected.add(index)
        continuation = directive and line.rstrip().endswith("\\")
    for index, line in enumerate(lines):
        if index in protected or line.strip() != "{" or not index:
            continue
        first = index - 1
        while first > 0 and lines[first - 1].strip():
            first -= 1
        block = "\n".join(lines[first:index])
        signature = re.sub(r"/\*.*?\*/|//[^\n]*", "", block, flags=re.DOTALL).strip()
        is_type = re.match(r"(?:typedef\s+)?(?:struct|class|enum|union)\b", signature)
        is_function = (
            signature.endswith(")")
            and re.search(r"[A-Za-z_~][\w:~]*\s*\(", signature)
            and not re.match(r"(?:if|else|for|while|switch|catch|return|do)\b", signature)
            and not any(char in signature.split("(", 1)[0] for char in "=;{}#")
        )
        if (is_type or is_function) and not protected.intersection(range(first, index + 1)):
            targets.add(first)
    # Public/private prototypes need the same spacing as their implementations.
    prototype = re.compile(r"^(?:[A-Za-z_][\w:]*[ \t*&]+)+[A-Za-z_~][\w:~]*[ \t]*\(")
    for index, line in enumerate(lines):
        stripped = line.strip()
        if index in protected or not prototype.match(stripped):
            continue
        if re.match(r"(?:return|if|else|while|for|switch|throw|delete|new)\b", stripped):
            continue
        first = index
        # Include the immediately attached block/line documentation in the spacing.
        if first and lines[first - 1].strip().endswith("*/"):
            first -= 1
            while first and "/*" not in lines[first]:
                first -= 1
        else:
            while first and lines[first - 1].lstrip().startswith("//"):
                first -= 1
        if not protected.intersection(range(first, index + 1)):
            targets.add(first)
    output = []
    for index, line in enumerate(lines):
        if index in targets and output:
            blanks = 0
            for previous in reversed(output):
                if previous.strip():
                    break
                blanks += 1
            output.extend([""] * max(0, 2 - blanks))
        if index not in protected and output and output[-1].lstrip().startswith("#include"):
            if line.strip() and not line.lstrip().startswith("#include"):
                output.append("")
        output.append(line)
    result = "\n".join(output) + "\n"
    # Keep the first scope/fixture explanation attached to its opening brace.
    # This preserves the tutorial contract that every code line is explained.
    return re.sub(
        r"(?m)^([^\n]*\{)[ \t]*\n[ \t]*(/\*[^\n]*?\*/)[ \t]*$",
        r"\1 \2",
        result,
    )


# Select only Git-tracked handwritten native files, including tests and examples.
# Parameters: none. Returns absolute Paths; no source or Git state is modified.
def tracked_sources():
    """Select owned native source; never modify vendored/generated upstream code."""
    names = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0")
    return [
        ROOT / name
        for name in names
        if name
        and name != "compiler/limbo/frontend/bindings_as_runtime.h"
        and Path(name).suffix in EXTENSIONS
        and not {"upstream", "external", "generated"}.intersection(Path(name).parts)
    ]


# Parse CLI options and apply/check the selected formatter with lexical safeguards.
# Parameters: none in Python; CLI selects executable, jobs and write mode.
# Returns zero on success, one for read-only drift; invalid changes raise an error.
def main():
    """Format/check selected files; return nonzero on drift or a rejected change."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formatter", default="clang-format")
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    version = subprocess.check_output([args.formatter, "--version"], text=True)
    if not re.search(r"version 18\.", version):
        parser.error("the checked profile requires clang-format 18.x")
    if args.jobs < 1:
        parser.error("--jobs must be positive")

    # Format one tracked path to a stable layout before any write.
    # Parameters: path is owned source. Returns (path, changed); invalid tokens fail.
    def process(path):
        """Check token preservation before optionally replacing one source file."""
        original = path.read_text()
        formatted = original
        # Spacing can split an alignment group; settle its layout before writing.
        for attempt in range(6):
            previous = formatted
            formatted = subprocess.check_output(
                [args.formatter, "--style=file", "--Werror", "--assume-filename=" + str(path)],
                input=previous,
                text=True,
            )
            formatted = declaration_spacing(formatted)
            if formatted == previous:
                break
        else:
            raise ValueError("formatter did not converge: " + str(path.relative_to(ROOT)))
        if native_tokens(original) != native_tokens(formatted):
            raise ValueError("non-layout token change: " + str(path.relative_to(ROOT)))
        if directive_tokens(original) != directive_tokens(formatted):
            raise ValueError("preprocessor boundary change: " + str(path.relative_to(ROOT)))
        changed = original != formatted
        if changed and args.write:
            path.write_text(formatted)
        return path, changed

    sources = tracked_sources()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(process, sources))
    changed = [str(path.relative_to(ROOT)) for path, drift in results if drift]
    print("Native files checked:", len(sources))
    print(
        "Native files changed" if args.write else "Native files requiring formatting", len(changed)
    )
    if not args.write:
        for name in changed:
            print(name)
    return int(bool(changed) and not args.write)


if __name__ == "__main__":
    raise SystemExit(main())
