# Copyright (c) 2026 bhgv
# EN: Annotate exported C contracts, preserve executable tokens, format and split long readable units.
# RU: Аннотировать C-контракты экспорта, сохранять исполнимые токены, форматировать и делить длинные читаемые units.
from pathlib import Path
import argparse
import ast
import json
import re
import subprocess
from .contracts import PURPOSES, file_role, parameter, returns

TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|(?:L|u8|u|U)?"(?:\\.|[^"\\])*"|(?:L|u|U)?\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|[0-9]+(?:\.[0-9]+)?|[^\s]', re.S)  # Literal-preserving C tokens.
PART = re.compile(r'^\s*#include "([^"\n]*readable_parts/[^"\n]+\.inc)"\s*$', re.M)  # Only this tool's split units are flattened.


# EN/RU: Flatten only generated readable-part includes of a borrowed path; return owned source text.
# Раскрыть только generated readable-part includes заданного пути; вернуть собственный текст.
def flattened(path):
    text = path.read_text()
    text = re.sub(r"/\* Copyright \(c\) 2026 bhgv — readability annotations only;.*?\*/\s*", "", text, flags=re.S)
    text = re.sub(r"/\* EN/RU: Readable sections compile as one original translation unit;.*?\*/\s*", "", text, flags=re.S)
    text = re.sub(r"/\* Copyright \(c\) 2026 bhgv — readability facade;.*?\*/\s*", "", text, flags=re.S)
    return PART.sub(lambda match: flattened(path.parent / match.group(1)), text)


# EN/RU: Remove only this tool's prior contracts before regenerating them, preserving original comments verbatim.
# Удалить только прежние контракты этого инструмента перед повторной генерацией, сохранив исходные комментарии.
def unannotated(path):
    return re.sub(r"/\*\s*\[standalone-contract:[^]]+\].*?\*/\s*", "", flattened(path), flags=re.S)


# EN/RU: Return non-comment C token spellings; braces/continuations may be added by the checked formatter.
# Вернуть C-токены без комментариев; braces/continuations могут добавляться проверенным formatter.
def tokens(text):
    return [match.group() for match in TOKEN.finditer(text)
            if not match.group().startswith(("/*", "//")) and match.group() not in ("{", "}", "\\")]


# EN: Compare exported algorithms; Python uses AST, C uses literal-preserving tokens, other assets remain byte-text exact.
# RU: Сравнить алгоритмы экспорта; Python по AST, C по токенам с литералами, прочие assets — точным текстом.
def same_tokens(original, target):
    actual = flattened(target)
    if target.suffix == ".py":
        return ast.dump(ast.parse(original), include_attributes=False) == ast.dump(ast.parse(actual), include_attributes=False)
    if target.suffix in (".c", ".h", ".inc"):
        return tokens(original) == tokens(actual)
    return original == actual


# EN/RU: Collect matching syntax nodes under a borrowed tree; definitions are not recursively duplicated.
# Собрать подходящие syntax nodes дерева; вложения определений повторно не выдаются.
def nodes(node, kind):
    if node.type == kind:
        yield node
    else:
        for child in node.children:
            yield from nodes(child, kind)


# EN/RU: Descend a C declarator to its identifier and outer parameter list; return borrowed syntax nodes.
# Спуститься по C declarator к имени и внешнему списку параметров; вернуть заимствованные nodes.
def declarator(node):
    params = None
    while node and node.type != "identifier":
        if node.type == "function_declarator" and params is None:
            params = node.child_by_field_name("parameters")
        node = node.child_by_field_name("declarator")
    return node, params


# EN/RU: Extract exact function metadata and adjacent original behavioral comments; data/tree are borrowed.
# Извлечь точные metadata функции и соседние исходные описания поведения; data/tree заимствованы.
def functions(data, tree, path):
    result = []
    for node in nodes(tree.root_node, "function_definition"):
        name, params = declarator(node.child_by_field_name("declarator"))
        if name is None:
            raise RuntimeError("unrecognized function declaration: " + str(path))
        spelling = data[name.start_byte:name.end_byte].decode()
        arguments = []
        if params:
            for param in params.named_children:
                if param.type == "parameter_declaration":
                    identifier, _ = declarator(param.child_by_field_name("declarator"))
                    raw = data[param.start_byte:param.end_byte].decode()
                    if raw != "void":
                        label = data[identifier.start_byte:identifier.end_byte].decode() if identifier else raw
                        arguments.append(dict(name=label, declaration=raw, descriptions=parameter(label, raw)))
                elif param.type == "variadic_parameter":
                    arguments.append(dict(name="...", declaration="...", descriptions=("Variadic arguments matching the supplied format.", "Переменные аргументы, соответствующие переданному format.")))
        kind = data[node.start_byte:name.start_byte].decode().strip()
        kind = re.sub(r"\b(?:static|inline|extern)\s*", "", kind).strip()
        body = data[node.child_by_field_name("body").start_byte:node.end_byte].decode()
        previous = node.prev_named_sibling
        original = []
        while previous and previous.type == "comment":
            value = data[previous.start_byte:previous.end_byte].decode()
            if "standalone-contract:" not in value and "Copyright" not in value:
                original.insert(0, re.sub(r"^\s*\* ?", "", value.strip("/* \n"), flags=re.M))
            previous = previous.prev_named_sibling
        role = file_role(path)
        purpose = PURPOSES.get(spelling, (f"{spelling}: {role[0]}.", f"{spelling}: {role[1]}."))
        result.append(dict(name=spelling, parameters=arguments, returns=returns(kind, body), kind=kind,
            syntax=data[node.start_byte:node.child_by_field_name("body").start_byte].decode().strip(),
            purpose=purpose, original="\n\n".join(original), private="static" in data[node.start_byte:name.start_byte].decode().split(),
            start=node.start_byte, row=name.start_point.row + 1))
    return result


# EN/RU: Build a bilingual parameter/return contract without deleting any original license or behavioral comment.
# Составить EN/RU контракт параметров/результата, не удаляя исходные лицензии и описания поведения.
def contract(info):
    lines = ["", "", f"/* [standalone-contract:{info['name']}]", " * EN: " + info["purpose"][0], " * RU: " + info["purpose"][1]]
    for argument in info["parameters"]:
        lines.append(f" * {argument['name']} ({argument['declaration']}): {argument['descriptions'][0]}")
        lines.append(" * RU: " + argument["descriptions"][1])
    if not info["parameters"]:
        lines.append(" * Parameters: none. Параметры: нет.")
    lines.extend([" * Return: " + info["returns"][0].replace("`", ""), " * Возврат: " + info["returns"][1].replace("`", ""),
                  " * Ownership/threading: retain the original contract immediately above and the public header's requirements.",
                  " * Владение/потоки: действуют исходный контракт выше и требования публичного header.", " */", ""])
    return "\n".join(lines)


# EN: Split a formatted source at preprocessor-balanced boundaries; includes preserve the original C token stream.
# Retain an outer include guard in its facade. Return the original-line to physical-file/line mapping.
# RU: Разделить форматированный исходник по сбалансированным preprocessor-границам, сохранив C-токены.
# Внешний include guard оставить в facade. Вернуть отображение исходных строк в физические file/line.
def split_source(path, text):
    import shutil
    shutil.rmtree(path.parent / "readable_parts" / path.name.replace(".", "-"), ignore_errors=True)
    lines = text.splitlines(keepends=True)
    if len(lines) <= 500:
        path.write_text(text)
        return {index + 1: (path, index + 1) for index in range(len(lines))}
    guard = re.search(r"^#ifndef\s+(\w+)\s*\n#define\s+\1[^\n]*\n", text, re.M)
    prefix = ""
    suffix = ""
    first = 0
    last = len(lines)
    if guard and lines[-1].strip().startswith("#endif"):
        first = text[:guard.end()].count("\n")
        prefix = "".join(lines[:first])
        suffix = lines[-1]
        last -= 1
    pieces = []
    start = first
    depth = 0
    continuation = False
    candidates = []
    protected = set()
    for token in TOKEN.finditer(text):
        if token.group().startswith("/*"):
            begin = text[:token.start()].count("\n")
            end = text[:token.end()].count("\n")
            protected.update(range(begin + 1, end + 1))
    for index in range(first, last):
        line = lines[index]
        stripped = line.lstrip()
        if not continuation and stripped.startswith(("#if ", "#if\t", "#ifdef", "#ifndef")):
            depth += 1
        if not continuation and stripped.startswith("#endif"):
            depth -= 1
        continuation = line.rstrip().endswith("\\")
        if depth == 0 and not continuation and index + 1 not in protected:
            candidates.append(index + 1)
        if index + 1 - start >= 430:
            choices = [cut for cut in candidates if start < cut <= index + 1]
            if not choices:
                if index + 1 - start >= 480:
                    raise RuntimeError("cannot split a balanced source unit: " + str(path))
                continue
            cut = choices[-1]
            pieces.append((start, cut))
            start = cut
            candidates = [cut for cut in candidates if cut > start]
    if start < last:
        pieces.append((start, last))
    directory = path.parent / "readable_parts" / path.name.replace(".", "-")
    directory.mkdir(parents=True, exist_ok=True)
    mapping = {}
    includes = []
    for number, (begin, end) in enumerate(pieces, 1):
        part = directory / f"part-{number:03}.inc"
        header = ("/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.\n"
                  f" * EN: Sequential implementation section of {path.name}; inherits the facade's C scope and original declarations.\n"
                  " * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */\n")
        part.write_text((header + "".join(lines[begin:end])).rstrip() + "\n")
        for row in range(begin, end):
            mapping[row + 1] = (part, row - begin + 4)
        includes.append('#include "' + part.relative_to(path.parent).as_posix() + '"\n')
    if not prefix:
        prefix = ("/* Copyright (c) 2026 bhgv — readability facade; original component notices remain in the parts below.\n"
                  f" * EN: Compile readable sections of {path.name} as one translation unit.\n"
                  " * RU: Компилировать читаемые части как одну translation unit, сохранив исходный алгоритм. */\n")
    path.write_text(prefix + "\n/* EN/RU: Readable sections compile as one original translation unit; no runtime linkage is added.\n"
        " * Читаемые части компилируются как одна исходная translation unit; новой runtime-линковки нет. */\n" + "".join(includes) + suffix)
    return mapping


# EN: Annotate/format all exported C files, verify preserved tokens, split long units and write exact reference inventories.
# CLI formatter is a development-only clang-format18 executable; packages do not invoke this tool at build time.
# RU: Аннотировать/форматировать C-файлы, проверить токены, разделить длинные units и записать точный reference inventory.
# CLI formatter — clang-format18 только для разработки; сборка пакетов этот инструмент не вызывает.
def main():
    from tree_sitter import Language, Parser
    import tree_sitter_c
    parser = argparse.ArgumentParser()
    parser.add_argument("--formatter", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    syntax = Parser(Language(tree_sitter_c.language()))
    for package in ("styx", "libunibus"):
        base = root / package
        inventory = []
        paths = sorted(path for path in base.rglob("*") if path.suffix in (".c", ".h") and "readable_parts" not in path.parts)
        definitions = {}
        for path in paths:
            data = unannotated(path).encode()
            for info in functions(data, syntax.parse(data), path.relative_to(base)):
                if not info["private"]:
                    definitions.setdefault(info["name"], info)
        for path in paths:
            source = unannotated(path)
            data = source.encode()
            found = functions(data, syntax.parse(data), path.relative_to(base))
            if path.suffix == ".h":
                for node in nodes(syntax.parse(data).root_node, "declaration"):
                    identifier, params = declarator(node.child_by_field_name("declarator"))
                    if identifier is not None and params is not None:
                        label = data[identifier.start_byte:identifier.end_byte].decode()
                        if label in definitions:
                            info = dict(definitions[label])
                            info["start"] = node.start_byte
                            found.append(info)
            for info in sorted(found, key=lambda value: value["start"], reverse=True):
                if "standalone-reviewed:" in source:
                    continue
                before = data[max(0, info["start"] - 3000):info["start"]]
                if f"[standalone-contract:{info['name']}]".encode() not in before:
                    data = data[:info["start"]] + contract(info).encode() + data[info["start"]:]
            introduction = data[:500].decode(errors="replace")
            if "Copyright" not in introduction or "EN:" not in introduction or "RU:" not in introduction:
                role = file_role(path.relative_to(base))
                data = ("/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.\n"
                        f" * EN: {role[0]}.\n * RU: {role[1]}.\n */\n").encode() + data
            result = subprocess.run([args.formatter, "--style=file", "--assume-filename=" + str(path)], input=data, capture_output=True)
            if result.returncode:
                raise RuntimeError(result.stderr.decode())
            formatted = result.stdout.decode()
            if tokens(source) != tokens(formatted):
                raise RuntimeError("formatter changed non-layout tokens: " + str(path))
            metadata = functions(formatted.encode(), syntax.parse(formatted.encode()), path.relative_to(base))
            mapping = split_source(path, formatted)
            for info in metadata:
                physical, line = mapping[info["row"]]
                info["source"] = physical.relative_to(base).as_posix()
                info["line"] = line
                info["unit"] = path.relative_to(base).as_posix()
                del info["start"]
                inventory.append(info)
        out = base / "docs" / "inventory"
        out.mkdir(parents=True, exist_ok=True)
        for part in range(0, len(inventory), 200):
            (out / f"functions-{part // 200:02}.jsonl").write_text("\n".join(json.dumps(info, ensure_ascii=False) for info in inventory[part:part + 200]) + "\n")
        print(package, "annotated", len(inventory), "functions in", len(paths), "files")


if __name__ == "__main__":
    main()
