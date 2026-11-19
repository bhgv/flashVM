# Copyright (c) 2026 bhgv
# EN: Human-authored shared terminology for native signatures; original function comments remain the behavioral authority.
# RU: Общая терминология native signatures; исходные комментарии функций остаются источником поведения.
import re

PURPOSES = {
    "convM2S": ("Decode one original 9P2000 message, validating counted fields inside its declared extent.", "Декодировать исходное 9P2000-сообщение с проверкой counted fields внутри объявленной длины."),
    "convS2M": ("Encode a native Fcall as standard 9P2000 bytes with explicit little-endian fields.", "Кодировать native Fcall в стандартные 9P2000 байты с явным little-endian полей."),
    "sizeS2M": ("Calculate the exact standard message size before encoding; reject unsupported or unrepresentable fields.", "Вычислить точный размер стандартного сообщения до кодирования; отклонить неподдерживаемые поля."),
    "convD2M": ("Encode one standard Dir record; a short buffer can receive only its two-byte required-size prefix.", "Кодировать стандартную Dir-запись; короткий буфер может получить только двухбайтовый prefix требуемой длины."),
    "sizeD2M": ("Calculate the complete standard stat-record size, including its leading size field.", "Вычислить полный размер стандартной stat-записи, включая начальное поле длины."),
    "convM2D": ("Decode a validated standard Dir record and copy its strings into caller-supplied storage when provided.", "Декодировать проверенную Dir-запись и скопировать строки в буфер вызывающего, если он задан."),
    "statcheck": ("Validate a complete encoded stat record and each counted string without exposing a partial record.", "Проверить целую stat-запись и каждую counted string, не выдавая частично разобранную запись."),
    "styxinit": ("Initialize an original Styx server, its root and selected platform listener.", "Инициализировать исходный Styx-сервер, его корень и listener выбранной платформы."),
    "styxprocess": ("Dispatch available original Styx messages through the selected server callbacks.", "Диспетчеризовать доступные исходные Styx-сообщения через callbacks сервера."),
    "styxwait": ("Wait for the selected server backend to make connection/message work available.", "Ожидать появления работы соединений/сообщений у выбранного backend сервера."),
    "styxend": ("Retire the original server, its clients and opened fid state through their cleanup callbacks.", "Завершить исходный сервер, клиентов и открытые fid через callbacks освобождения."),
    "styxclient": ("Borrow the current callback's original client context from its server.", "Заимствовать текущий original Client-контекст callback из его сервера."),
    "styxqid": ("Construct a scalar Qid from a stable backend identity and directory flag.", "Составить скалярный Qid из стабильного ID backend и признака каталога."),
    "catalog_query": ("Query one live stat or a bounded immediate-child page without traversing the whole namespace.", "Запросить живой stat либо ограниченную страницу непосредственных детей без обхода всего namespace."),
    "main": ("Validate the program's command-line contract, own its resources and return a process exit status.", "Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения."),
}
FILES = {
    "srecord": ("canonical integers, bounded S-record framing and checksum validation", "канонические целые, framing S-записей и проверка checksum"),
    "sstream": ("incremental checked S-record input and caller-buffer lifetime", "инкрементальный проверяемый ввод S-записей и время жизни буфера"),
    "sbindings": ("direction-local numeric tag to schema identity bindings", "привязки числовых тегов направления к идентичности схемы"),
    "scontrol": ("correlated per-tag size/profile negotiation state", "коррелированное согласование размера/профиля отдельного тега"),
    "sslot": ("checked fixed-slot geometry and compact-record extents", "проверяемая геометрия fixed slots и размеры compact records"),
    "smedia": ("bounded media-fragment framing and caller-owned reassembly", "ограниченное framing media fragments и сборка в буфере вызывающего"),
    "smeasure": ("per-stream measurements and explicit adaptation inputs", "измерения потока и явные входы адаптации"),
    "sadapt": ("host-selected link adaptation decisions and hysteresis", "выбранные host решения адаптации связи и гистерезис"),
    "prefix": ("unsigned64 ordinal widths and CRC5 validation", "ширины unsigned64 номеров и проверка CRC5"),
    "packet": ("checked ordinal prefixes around unchanged S-record bytes", "проверяемые префиксы номеров вокруг неизменённых S-record байт"),
    "control": ("named order-service offer/accept/renewal records", "именованные order-service offer/accept/renewal записи"),
    "styxserver": ("original server session/fid dispatch and callback-backed namespace operations", "исходная диспетчеризация сессий/fid и callback-операции namespace"),
    "Posix": ("POSIX listener, readiness and socket byte transport for the original server", "POSIX listener, readiness и socket byte transport исходного сервера"),
    "dtoa": ("original decimal conversion with checked integer limbs and synchronized public entry points", "исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами"),
    "fmt": ("original format conversion registry and native callback publication", "исходный реестр formatter и публикация нативных callbacks"),
    "cleanname": ("lexical path normalization without filesystem lookup", "лексическая нормализация пути без обращения к файловой системе"),
    "rune": ("UTF-8 byte/rune conversion and validity", "преобразование и проверка UTF-8 byte/rune"),
    "runetype": ("Unicode rune classification and case mapping", "классификация Unicode rune и преобразование регистра"),
}
NAMES = {
    "size": ("Requested byte extent; the relevant buffer must cover it.", "Запрошенная длина в байтах; соответствующий буфер должен её покрывать."),
    "capacity": ("Actual caller-owned output capacity; it is not permission to write past the buffer.", "Фактическая ёмкость буфера вызывающего; выход за неё недопустим."),
    "limit": ("Explicit local quota; permitted ranges and wire/body units are given in the description.", "Явная локальная квота; диапазон и единицы wire/body указаны в описании."),
    "offset": ("Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.", "Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл."),
    "tag": ("Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.", "ID этого направления/пространства; внешний0 зарезервирован для служебного трафика."),
    "ordinal": ("Unsigned sequence value for the selected tag/direction, never a machine address.", "Беззнаковый номер выбранного тега/направления, не адрес машины."),
    "timeout": ("Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.", "Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом."),
    "mode": ("Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.", "Режим операции/доступа; использовать протокольные константы, не суффикс имени."),
    "state": ("Opaque callback context; its lifetime/transfer is specified by the creating API.", "Непрозрачный контекст callback; время жизни/передача заданы создающим API."),
    "path": ("Selected object path or stable identity according to the declared type; no implicit authority is granted.", "Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав."),
    "name": ("Component or schema name in this operation; the function validates its own encoding/length limits.", "Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины."),
    "used": ("Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.", "Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже."),
    "argc": ("Number of command-line entries, including the program name.", "Число аргументов командной строки вместе с именем программы."),
    "argv": ("Process-lifetime argument strings; accepted options are described by the example README.", "Строки аргументов, живущие до конца процесса; варианты описаны в README примера."),
}
CONTEXTS = {
    "StyxCancel": ("Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.", "Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом."),
    "StyxTransfer": ("Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.", "Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект."),
    "StyxClient": ("Native client/session context; retain or consume it exactly as specified by this API.", "Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API."),
    "StyxFid": ("Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.", "Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами."),
    "Styxserver": ("Original server instance and dispatcher-owned state.", "Экземпляр исходного сервера и состояние его dispatcher."),
    "Client": ("Original connection context, valid only while its owning server retains it.", "Исходный контекст соединения, действующий пока сервер его удерживает."),
    "StyxOrder": ("Native per-tag queue engine; keep it alive until all concurrent calls retire.", "Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов."),
    "RemotePhy": ("Physical-carrier capability; ownership transfer depends on the constructor/operation.", "Capability физического носителя; передача владения зависит от конструктора/операции."),
    "Fmt": ("Active original formatter context, including its output cursor and conversion state.", "Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования."),
    "Dir": ("Native metadata/string references; encode fields rather than transmitting the structure layout.", "Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры."),
    "Qid": ("Stable backend object identity/version/kind, not a native pointer or path string.", "Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути."),
    "WalkScope": ("Request-local active traversal guard; successful entry requires its matching leave.", "Локальный guard активного обхода запроса; успешный enter требует соответствующего leave."),
    "FVM_S": ("Caller-owned protocol value/state; native layout is never a wire representation.", "Значение/состояние протокола вызывающего; native layout не является wire-представлением."),
    "Rune": ("Unicode rune value or rune buffer, distinct from a UTF-8 byte count.", "Unicode rune либо буфер rune, отличный от числа UTF-8 байт."),
}


# EN/RU: Return bilingual file responsibility using its actual subsystem/stem; path is a package-relative source path.
# Вернуть описание файла EN/RU по подсистеме/stem; path — относительный путь исходника пакета.
def file_role(path):
    if path.stem in FILES:
        return FILES[path.stem]
    if "lib9" in path.parts:
        return (f"original POSIX lib9 {path.stem} support", f"исходная POSIX-поддержка lib9 {path.stem}")
    if "client" in path.parts:
        return (f"native client {path.stem} operations and lifetime", f"нативные client-операции {path.stem} и время жизни")
    if "order" in path.parts:
        return (f"independent per-tag {path.stem} state", f"независимое состояние тегов {path.stem}")
    return (f"{path.parent.name}/{path.stem} implementation and native state", f"реализация {path.parent.name}/{path.stem} и нативное состояние")


# EN/RU: Describe one exact typed parameter; specialize known contexts and avoid inventing ownership from a pointer alone.
# Описать точный typed parameter; уточнить известные контексты, не выводить владение только из наличия указателя.
def parameter(name, declaration):
    for context, text in CONTEXTS.items():
        if context in declaration:
            return text
    if name in NAMES:
        return NAMES[name]
    if name in ("buf", "bytes", "input", "output", "out", "ap", "p", "ep", "strs") and "*" in declaration:
        return ("Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.", "Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.")
    if name in ("n", "nbuf", "nap", "count", "len", "length"):
        return ("Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.", "Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.")
    if "*" in declaration:
        return ("Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.", "Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.")
    return ("Scalar operand of this operation; protocol/algorithm ranges are described below.", "Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.")


# EN/RU: Describe the actual return type and literal result constants; never infer success from truthiness alone.
# Описать фактический возвращаемый тип и literal constants; не выводить успех только из ненулевого значения.
def returns(kind, body):
    values = sorted(set(re.findall(r"\breturn\s+((?:FVM_|STYX_|WALK_)[A-Z_]+|NULL|nil|-?[0-9]+)\s*;", body)))
    if kind.strip() == "void":
        return ("No C return value; output/state/ownership effects are specified in Description.", "Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.")
    literals = ", ".join(values) if values else "see Description and the implementation"
    return (f"`{kind}`; direct result constants in this body: {literals}. Forwarded results preserve the called API's status.",
            f"`{kind}`; прямые константы результата тела: {literals}. Пересылаемый результат сохраняет статус вызываемого API.")
