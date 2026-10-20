#!/usr/bin/env python3
"""Format Coco/R C grammars without moving semantic actions across parser tokens.

Semantic actions end their physical line and start at column 73. Grammar groups
use four-space nesting; rule periods stand alone and are followed by a blank.
The lexer treats C strings/comments as opaque, including apparent .) delimiters.
Coco/R remains the parser generator: this tool only changes source layout.
"""
import argparse
from pathlib import Path
import re
import subprocess
import textwrap


ACTION_COLUMN = 72
SECTIONS = {'CHARACTERS', 'TOKENS', 'PRAGMAS', 'PRODUCTIONS', 'IGNORE', 'END'}


def quoted(text, start):
    """Find a complete string/character literal, preserving escaped delimiters."""
    quote, pos = text[start], start + 1
    while pos < len(text):
        if text[pos] == '\\':
            pos += 2
        elif text[pos] == quote:
            return pos + 1
        else:
            pos += 1
    raise ValueError('unterminated quoted literal')


def c_region(text, start, ending=None):
    """Skip C strings/comments until an action terminator or balanced close."""
    pos, depth = start, 1
    while pos < len(text):
        if text.startswith('/*', pos):
            end = text.find('*/', pos + 2)
            if end < 0: raise ValueError('unterminated C comment')
            pos = end + 2
        elif text.startswith('//', pos):
            end = text.find('\n', pos)
            pos = len(text) if end < 0 else end
        elif text[pos] in "\"'":
            pos = quoted(text, pos)
        elif ending and text.startswith(ending, pos):
            return pos + len(ending)
        elif not ending and text[pos] == '(':
            depth += 1
            pos += 1
        elif not ending and text[pos] == ')':
            depth -= 1
            pos += 1
            if not depth: return pos
        else:
            pos += 1
    raise ValueError('unterminated C action/predicate')


def tokens(text):
    """Lex EBNF, opaque C actions/attributes, and comments in original order."""
    pos = 0
    while pos < len(text):
        if text[pos].isspace():
            pos += 1
            continue
        start = pos
        if text.startswith('/*', pos):
            end = text.find('*/', pos + 2)
            if end < 0: raise ValueError('unterminated grammar comment')
            pos = end + 2
            kind = 'comment'
        elif text.startswith('//', pos):
            end = text.find('\n', pos)
            pos = len(text) if end < 0 else end
            kind = 'comment'
        elif text.startswith('(.', pos):
            pos = c_region(text, pos + 2, '.)')
            kind = 'action'
        elif text[pos] in "\"'":
            pos = quoted(text, pos)
            kind = 'token'
        elif text[pos] == '<':
            # Coco/R attributes in these grammars contain C99 argument lists.
            end = text.find('>', pos + 1)
            if end < 0: raise ValueError('unterminated grammar attribute')
            pos = end + 1
            kind = 'attribute'
        elif text.startswith('..', pos):
            pos += 2
            kind = 'token'
        else:
            match = re.match(r'[A-Za-z_][A-Za-z_0-9]*', text[pos:])
            if match:
                pos += len(match[0])
                following = re.match(r'\s*\(', text[pos:])
                if match[0] in ('IF', 'CHR') and following:
                    pos = c_region(text, pos + len(following[0]))
            else:
                pos += 1
            kind = 'token'
        yield kind, text[start:pos]


def c_format(text, executable):
    """Optionally apply an explicitly provided host clang-format, never a target tool."""
    if not executable: return textwrap.dedent(text).strip()
    run = subprocess.run([executable, '--style={BasedOnStyle: LLVM, IndentWidth: 4, '
                          'BreakBeforeBraces: Allman, ColumnLimit: 88}'],
                         input=textwrap.dedent(text).strip(), text=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    return run.stdout.strip()


def format_grammar(source, clang_format=None):
    """Preserve lexical order while making parser nesting and C actions explicit."""
    split = re.search(r'^CHARACTERS\s*$', source, re.M)
    compiler = re.search(r'^COMPILER\s+\w+[^\n]*\n', source, re.M)
    if not split or not compiler: raise ValueError('missing COMPILER/CHARACTERS section')
    prefix = source[:compiler.end()].rstrip() + '\n'
    prelude = source[compiler.end():split.start()]
    prefix += c_format(prelude, clang_format) + '\n\n'
    output, current, depth, rule = [], '', 0, False

    def flush():
        """Finish one parser-only line without trailing whitespace."""
        nonlocal current
        if current.strip(): output.append(current.rstrip())
        current = ''

    def put(value, indent=None, attach=False):
        """Append one grammar token, wrapping before the semantic-action column."""
        nonlocal current
        if not current: current = ' ' * (indent if indent is not None else 4 + depth * 4)
        separator = '' if attach or not current.strip() else ' '
        if len(current + separator + value) > ACTION_COLUMN - 4 and current.strip():
            flush()
            current = ' ' * (4 + depth * 4)
            separator = ''
        current += separator + value

    for kind, value in tokens(source[split.start():]):
        if kind == 'comment':
            flush()
            output.extend('  ' + line.strip() for line in value.splitlines())
        elif kind == 'action':
            body = c_format(value[2:-2], clang_format)
            if len(current.rstrip()) > ACTION_COLUMN - 4: flush()
            leader = current.rstrip().ljust(ACTION_COLUMN)
            lines = body.splitlines()
            if len(lines) <= 1:
                output.append(leader + '(. ' + body + ' .)')
            else:
                output.append(leader + '(.')
                output.extend(' ' * (ACTION_COLUMN + 4) + line for line in lines)
                output.append(' ' * ACTION_COLUMN + '.)')
            current = ''
        elif value in SECTIONS:
            flush()
            if output and output[-1]: output.append('')
            output.append(value)
            depth, rule = 0, False
        elif value == '.':
            if depth: raise ValueError('rule period inside unclosed EBNF group')
            flush()
            output.extend(['  .', ''])
            depth, rule = 0, False
        elif value == '=':
            put(value)
            flush()
            rule = True
        elif value in ('(', '[', '{'):
            flush()
            put(value)
            flush()
            depth += 1
        elif value in (')', ']', '}'):
            flush()
            depth -= 1
            if depth < 0: raise ValueError('unbalanced EBNF group')
            put(value)
            flush()
        elif value == '|':
            flush()
            put(value)
        else:
            put(value, indent=2 if not rule and not depth else None, attach=kind == 'attribute')
    flush()
    if depth: raise ValueError('unclosed EBNF group')
    return prefix + '\n'.join(output).rstrip() + '\n\n'


def c_tokens(text):
    """Ignore layout/comments while preserving C token and literal spellings."""
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|[0-9]+|>>=|<<=|\.\.\.|->|\+\+|--|==|!=|<=|>=|&&|\|\||<<|>>|[+*/%&|^-]=|\S'
    return tuple(m[0] for m in re.finditer(pattern, text, re.S)
                 if not m[0].startswith(('/*', '//')))


def semantic_signature(source):
    """Guard mechanical edits: compare prelude, grammar and semantic C tokens."""
    compiler = re.search(r'^COMPILER\s+\w+[^\n]*\n', source, re.M)
    section = re.search(r'^CHARACTERS\s*$', source, re.M)
    if not compiler or not section: raise ValueError('missing grammar sections')
    result = [('prelude', c_tokens(source[compiler.end():section.start()]))]
    for kind, value in tokens(source[section.start():]):
        if kind == 'comment': continue
        if kind == 'action': value = c_tokens(value[2:-2])
        elif kind == 'attribute' or value.startswith(('IF(', 'IF (', 'CHR(', 'CHR (')):
            value = c_tokens(value)
        result.append((kind, value))
    return tuple(result)


def main():
    """Require explicit paths; --check is read-only and does not need clang-format."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--clang-format')
    parser.add_argument('paths', nargs='+', type=Path)
    args = parser.parse_args()
    stale = []
    for path in args.paths:
        original = path.read_text(encoding='utf-8')
        formatted = format_grammar(original, args.clang_format)
        if semantic_signature(original) != semantic_signature(formatted):
            raise ValueError(str(path) + ': formatting changed token order')
        if original != formatted:
            stale.append(str(path))
            if not args.check: path.write_text(formatted, encoding='utf-8')
    print(str(len(stale)) + (' noncanonical' if args.check else ' formatted') + ' ATG files')
    if args.check and stale:
        print('\n'.join(stale))
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
