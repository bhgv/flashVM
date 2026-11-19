# Copyright (c) 2026 bhgv
# EN: Build bilingual dictionary pages from exact public declarations, preserved contracts and source inventories.
# RU: Строить EN/RU словарные страницы из точных публичных declarations, сохранённых контрактов и source inventory.
from pathlib import Path
import json
import re
from functools import lru_cache
from .layout import TOKEN, flattened, unannotated as read_unannotated

unannotated = lru_cache(maxsize=None)(read_unannotated)  # A generation pass does not modify source headers.
from .reference_notes import PARAMETERS, NOTES

CODECS = {"convM2S", "convS2M", "sizeS2M", "convD2M", "convM2D", "sizeD2M", "statcheck"}  # Exact original portable entry points.
CODEC_NOTES = {
    "convM2S": ("ap is mutable message storage of nap bytes. Counted strings are shifted in place to add NUL terminators. Fcall strings/data borrow ap: retain that storage until processing finishes. A zero return means incomplete/invalid input; discard the decoded object rather than trusting partial fields. Success is the declared message extent, not sizeof(Fcall).",
                  "ap — изменяемый буфер сообщения длиной nap. Counted strings сдвигаются на месте для NUL-терминаторов. Строки/data Fcall заимствуют ap до конца обработки. Ноль означает неполный/неверный вход: частичным полям доверять нельзя. Успех возвращает длину сообщения, не sizeof(Fcall)."),
    "convS2M": ("f and all strings/data it references remain caller-owned. ap has nap bytes. The codec validates type and counted-field extents before writing; zero means unsupported/malformed fields or insufficient capacity. It does not allocate, send a request, advance a fid or perform a filesystem operation.",
                  "f и строки/data остаются у вызывающего; ap имеет ёмкость nap. До записи проверяются тип и размеры counted fields. Ноль означает неподдерживаемые/неверные поля либо недостаточный буфер. Codec не выделяет память, не отправляет запрос и не выполняет операцию файловой системы."),
    "sizeS2M": ("Calculate capacity before convS2M; zero is an invalid/unsupported or overflowing message, not a successful empty frame. Every standard frame includes its explicit size/type/tag header.",
                    "Рассчитать ёмкость до convS2M; ноль означает неверное/неподдерживаемое либо переполняющее сообщение, не пустой успешный frame. У стандартного frame есть явные size/type/tag."),
    "convD2M": ("Call sizeD2M first when a complete record is required. If the buffer has at least two bytes but cannot hold the complete valid record, the original API writes its required-length prefix and returns 2. That is not a complete directory entry. Zero reports an invalid record or less than two bytes of capacity. Directory read adapters must emit whole records only.",
                  "Для полной записи сначала вызвать sizeD2M. Если буфер содержит минимум2 байта, но целая допустимая запись не помещается, исходный API пишет prefix требуемой длины и возвращает2. Это не целая запись каталога. Ноль — неверная запись либо ёмкость менее2. Directory read должен выдавать только целые записи."),
    "sizeD2M": ("The result includes the leading two-byte count. Each string has its own uint16 count; oversized strings/combined record lengths return zero. Directory offsets advance by this complete encoded size, never by the number of children.",
                    "Результат включает начальный двухбайтовый count. У строк собственные uint16 lengths; слишком длинные строки/запись дают ноль. Offset каталога увеличивается на полный кодированный размер, не на количество детей."),
    "convM2D": ("The input record is bounded by nbuf. Supply caller-owned strs storage large enough for all decoded strings and their terminators; using nbuf bytes is sufficient for a validated record. NULL strs requests scalar metadata with empty string fields. No strings are allocated by the codec. The returned extent is one decoded record; zero rejects invalid/truncated input.",
                  "Вход ограничен nbuf. В strs нужен собственный буфер для всех строк с терминаторами; nbuf байт достаточно для проверенной записи. NULL strs запрашивает скалярные metadata с пустыми строками. Codec строки не выделяет. Возвращается размер одной записи; ноль отклоняет неверный/усечённый вход."),
    "statcheck": ("Return 0 for one valid complete stat record, -1 for malformed extent/counts. This is structural validation, not an rwx/authentication decision and not a check that the backend object still exists.",
                   "Вернуть0 для целой допустимой stat-записи, -1 при неверных размерах/counts. Это структурная проверка, не решение rwx/auth и не проверка существования объекта backend."),
}


# EN/RU: Select only public package headers, excluding private implementation/test declarations.
# Выбрать только публичные headers пакета, исключив private implementation/test declarations.
def public_headers(base):
    if base.name == "libunibus":
        return list((base / "include").rglob("*.h")) + [base / "order/order.h"]
    return list((base / "include").rglob("*.h")) + [base / name for name in (
        "client/client.h", "server/phy/server.h", "remote_phy/phy.h", "remote_phy/drivers/posix/fd.h",
        "walk/walk.h", "walk/packet.h", "vendor/inferno/include/styxserver.h",
        "vendor/inferno/include/fcall.h", "vendor/inferno/Linux/386/include/lib9.h")]


# EN: Return cached declaration-search text from borrowed header path. Mask comments
# and quoted literals without changing offsets/newlines, so prose cannot impersonate a declaration.
# RU: Вернуть кешированный текст поиска declarations для header path. Заменить comments/
# literals пробелами, сохранив offsets/newlines: prose не должна имитировать declaration.
@lru_cache(maxsize=None)
def header_code(path):
    return TOKEN.sub(lambda match: re.sub(r"[^\n]", " ", match.group())
                     if match.group().startswith(("/*", "//")) or '"' in match.group() or "'" in match.group()
                     else match.group(), unannotated(path))


# EN: Return the first actual header declaration of borrowed name, or None.
# headers is an ordered iterable of readable source paths; comments/literals cannot match.
# RU: Вернуть первый header declaration имени name либо None. headers — упорядоченные
# пути; комментарии/литералы не могут выдаваться за declaration.
def declaration_header(headers, name):
    return next((path for path in headers if re.search(r"\b" + re.escape(name) + r"\s*\(", header_code(path))), None)


# EN/RU: Find the original pre-declaration comment, stripping generated contracts; return owned prose or empty text.
# Найти исходный комментарий перед declaration без generated contract; вернуть текст либо пустую строку.
def header_contract(headers, name):
    for header in headers:
        source = unannotated(header)
        masked = header_code(header)
        match = re.search(r"\b" + re.escape(name) + r"\s*\(", masked)
        if match:
            prefix = source[:match.start()]
            comments = list(re.finditer(r"/\*.*?\*/", prefix, re.S))
            if comments and len(re.sub(r"/\*.*?\*/", "", prefix[comments[-1].end():], flags=re.S).strip()) < 140:
                value = comments[-1].group()
                if "Copyright" not in value:
                    return re.sub(r"^\s*\* ?", "", value.strip("/* \n"), flags=re.M)
    return ""


# EN/RU: Describe the real target/platform selection, separately from implementation-level function visibility.
# Описать фактический выбор target/platform отдельно от видимости implementation-функции.
def availability(package, info):
    if package == "libunibus":
        return "Unibus::queues; C99 + pthread" if info["unit"].startswith("order/") else "Unibus::wire; C89, no OS/allocator/threads"
    if info["name"] in CODECS:
        return "Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build"
    for prefix, target in (("client/", "client"), ("server/phy/", "phy_server"), ("remote_phy/", "carrier"), ("walk/", "walk"), ("protocol/", "wire")):
        if info["unit"].startswith(prefix):
            return "Styx::" + target + ("; C89" if target == "wire" else "; hosted native build")
    return "Styx::server; hosted native build" if "libstyx" in info["unit"] else "Styx::nine; hosted Linux64 support"


# EN: Index first real call sites in borrowed base once per generation, preferring examples.
# Comments and quoted literals are excluded from matching; snippets preserve literal arguments.
# Returns owned name -> (relative path, physical line, code) entries; no source is modified.
# RU: Один раз индексировать первые реальные вызовы в base, предпочитая примеры.
# Не искать вызовы в комментариях/литералах; в snippet сохранить аргументы-литералы.
# Вернуть name -> (относительный путь, физическая строка, код); исходники не менять.
@lru_cache(maxsize=None)
def call_sites(base):
    paths = sorted(base.rglob("*.c")) + sorted(base.rglob("*.inc"))
    paths.sort(key=lambda path: ("examples" not in path.parts, "tests" not in path.parts, str(path)))
    result = {}
    for path in paths:
        source = path.read_text()
        code = TOKEN.sub(lambda match: re.sub(r"[^\n]", " ", match.group())
                         if match.group().startswith(("/*", "//")) else match.group(), source)
        scan = TOKEN.sub(lambda match: re.sub(r"[^\n]", " ", match.group())
                         if '"' in match.group() or match.group().startswith("'") else match.group(), code)
        for number, (line, search) in enumerate(zip(code.splitlines(), scan.splitlines()), 1):
            if search.lstrip().startswith(("#", "extern")) or ";" not in search:
                continue
            for name in re.findall(r"\b([A-Za-z_][A-Za-z_0-9]*)\s*\(", search):
                result.setdefault(name, (path.relative_to(base).as_posix(), number, line.strip()))
    return result


# EN: Return a cached concrete call for borrowed base/name, or None if no call site exists.
# RU: Вернуть кешированный реальный вызов для base/name либо None, если вызова нет.
def example(base, name):
    return call_sites(base).get(name)


# EN: Generate separate EN/RU dictionary pages for all actual public definitions; keep source anchors exact.
# RU: Создать отдельные EN/RU словарные страницы всех реальных публичных определений с точными source anchors.
def main():
    root = Path(__file__).resolve().parents[1]
    for package in ("styx", "libunibus"):
        base = root / package
        headers = public_headers(base)
        declarations = "\n".join(unannotated(path) for path in headers)
        declarations = re.sub(r"/\*.*?\*/|//[^\n]*", "", declarations, flags=re.S)
        names = set(re.findall(r"\b([A-Za-z_][A-Za-z_0-9]*)\s*\(", declarations))
        records = []
        for path in sorted((base / "docs/inventory").glob("*.jsonl")):
            records.extend(json.loads(line) for line in path.read_text().splitlines())
        chosen = {}
        for info in records:
            if not info["private"] and info["name"] in names and "examples/" not in info["unit"] and "/tests/" not in info["unit"]:
                chosen.setdefault(info["name"], info)
        directory = base / "docs/reference"
        directory.mkdir(parents=True, exist_ok=True)
        links = [[], []]
        for name, info in sorted(chosen.items()):
            description = header_contract(headers, name) or info["original"]
            declaring = declaration_header(headers, name)
            header_name = declaring.relative_to(base).as_posix() if declaring else info["source"]
            include_name = header_name.removeprefix("include/")
            if header_name.startswith("vendor/"):
                include_name = declaring.name
            elif header_name.startswith("client/"):
                include_name = "styx/" + header_name
            elif header_name.startswith("remote_phy/"):
                include_name = "styx/phy/" + header_name.removeprefix("remote_phy/")
            elif header_name.startswith("walk/"):
                include_name = "styx/" + header_name
            elif header_name.startswith("server/phy/"):
                include_name = "styx/server/server.h"
            elif header_name == "order/order.h":
                include_name = "unibus/order.h"
            if name in CODECS and package == "styx":
                include_name = "styx/codec.h"
            call = example(base, name)
            if package == "libunibus":
                help_path = "disVM/native/terminal/bridge/ORDERED.md" if "order" in info["unit"] else "flashVM/examples/embedding/srecords/README.md"
            else:
                help_path = "disVM/native/client/README.md" if info["unit"].startswith("client/") else "remote_phy/README.md" if info["unit"].startswith("remote_phy/") else "disVM/native/README.md"
            for language in (0, 1):
                suffix = ".ru.md" if language else ".md"
                title = ("Syntax", "Синтаксис", "Parameters", "Параметры", "Description", "Описание", "Return value", "Возвращаемое значение",
                         "Errors and ownership", "Ошибки и владение", "Availability", "Доступность", "Example", "Пример", "See also", "См. также")
                sections = [title[index + language] for index in range(0, len(title), 2)]
                lines = [f"# {name}", "", "Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.", "",
                    f"[{'English' if language else 'Русский'}]({name}{'.md' if language else '.ru.md'}) · [Index](../index{suffix})", "",
                    "## " + sections[0], "", "```c", ("#include <lib9.h>\n" if include_name in ("styxserver.h", "fcall.h", "styx.h") else "") + "#include <" + include_name + ">", "", info["syntax"] + ";", "```", "",
                    "## " + ("Библиотека" if language else "Library"), "", availability(package, info), "", "## " + sections[1], "",
                    ("| Параметр | Объявление | Назначение и ограничения |" if language else "| Parameter | Declaration | Meaning and constraints |"), "| --- | --- | --- |"]
                for argument in info["parameters"]:
                    meaning = PARAMETERS.get(name, {}).get(argument["name"], argument["descriptions"])[language]
                    lines.append(f"| `{argument['name']}` | `{argument['declaration']}` | {meaning} |")
                if not info["parameters"]:
                    lines.append("| — | void | " + ("No parameters." if not language else "Параметров нет.") + " |")
                lines.extend(["", "## " + sections[2], "", info["purpose"][language], ""])
                if name in NOTES:
                    lines.extend([NOTES[name][language], ""])
                if name in CODEC_NOTES:
                    lines.extend([CODEC_NOTES[name][language], ""])
                if description:
                    lines.extend(["### " + ("Preserved implementation/header contract" if not language else "Сохранённый контракт implementation/header"), "", description, ""])
                lines.extend(["## " + sections[3], "", info["returns"][language], "", "## " + sections[4], "",
                    ("Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write."
                     if not language else "Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически."), "",
                    "## " + sections[5], "", availability(package, info), "", "## " + sections[6], ""])
                if call:
                    source, number, snippet = call
                    lines.extend([("Actual call-site excerpt; its enclosing source provides variable initialization and cleanup." if not language else "Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике."), "", "```c", snippet, "```", "", f"[Complete calling context](../../{source}#L{number})", ""])
                else:
                    lines.extend([("Use the complete layer examples; this lower-level entry is also documented at its exact implementation below." if not language else "Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже."), "", "[Examples](../../examples/README" + suffix + ")", ""])
                lines.extend(["## " + sections[7], "", f"- [Implementation: {info['source']}:{info['line']}](../../{info['source']}#L{info['line']})",
                    f"- [Public declaration](../../{header_name})", f"- [API index](../index{suffix})", f"- [Protocol and lifetime](../protocol{suffix})", f"- [Main-project help](https://github.com/bhgv/flashVM/blob/master/{help_path})", ""])
                (directory / (name + suffix)).write_text("\n".join(lines))
                links[language].append(f"- [{name}](reference/{name}{suffix}) — `{availability(package, info)}`")
        for language in (0, 1):
            suffix = ".ru.md" if language else ".md"
            (base / "docs" / ("index" + suffix)).write_text("# " + ("Справочник API" if language else "API dictionary") + "\n\nCopyright (c) 2026 bhgv\n\n"
                + ("Отдельная страница для каждого фактически определённого публичного входа. Синтаксис и source anchors взяты из собираемой копии.\n\n" if language else "Separate pages cover actual public definitions. Syntax and source anchors come from the built source copy.\n\n")
                + f"[Guide](../README{suffix}) · [Introduction](introduction{suffix}) · [Tutorial](tutorial{suffix}) · [Examples](../examples/README{suffix}) · [Protocol](protocol{suffix}) · [Porting](porting{suffix})\n\n"
                + "\n".join(links[language]) + "\n")
        print(package, len(chosen), "public function pages per language")


if __name__ == "__main__":
    main()
