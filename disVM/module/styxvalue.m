# Copyright (c) 2026 bhgv
# EN: Typed values for the existing named StyxRPC frame format.
# RU: Типизированные значения существующего формата именованных кадров StyxRPC.
Styxvalue: module
{
    PATH: con "styxvalue.dis"; # Installed Limbo codec.
    Snapshot, Intref, Number, Text, Boolean, Null, Undefined, Cellref: con iota+1; # Existing wire tags.
    Tablekind: con 0; # Native table; named and numeric slots are distinct.
    Arraykind: con 28; # Standard Flash Array with a logical length.
    Objectkind: con 30; # Standard Flash Object.
    Maxframe: con 65536; # Maximum byte count including the tag.

    Value: adt
    {
        pick {
        Numeric =>
            value: real; # Exact IEEE binary64 value, including signed zero.
        Textual =>
            bytes: array of byte; # Counted UTF-8/WTF-8, including NUL.
        Logical =>
            value: int; # Zero or one.
        Nullval or Undefinedval =>
            reserved: int; # Zero, carries no payload.
        Graph =>
            bytes: array of byte; # Checked FVMG envelope, excluding the named tag.
        Link =>
            index: int; # Object index in a DataGraph, valid only inside a graph.
        }
    };

    Field: adt
    {
        name: string; # Counted UTF-8 name.
        value: ref Value; # Scalar or graph link.
    };

    Slot: adt
    {
        key: big; # Unsigned 32-bit sparse index.
        value: ref Value; # Scalar or graph link.
    };

    Node: adt
    {
        kind: int; # Tablekind, Arraykind or Objectkind.
        length: big; # Logical length for Arraykind; zero otherwise.
        fields: array of Field; # Own named entries, preserving their order.
        slots: array of Slot; # Own indexed entries, preserving their order.
    };

    DataGraph: adt
    {
        root: ref Value; # Root scalar or graph link.
        nodes: array of ref Node; # Nodes addressed by index. Unsupported unreachable entries may be nil.
    };

    # EN: Inspect portable own data in an FVMG v8/v9 snapshot. Nil for another profile.
    # RU: Прочитать переносимые собственные данные из FVMG v8/v9. Для другого профиля вернуть nil.
    inspect: fn(bytes: array of byte): ref DataGraph;

    # EN: Build an owned v8 snapshot with reachable data and shared graph links.
    # RU: Собрать собственный снимок v8 с достижимыми данными и общими ссылками графа.
    build: fn(graph: ref DataGraph): ref Value;

    # EN: Load Math before using the codec.
    # RU: Загрузить Math перед использованием кодека.
    init: fn();

    # EN: Wrap a Limbo string with counted UTF-8 bytes.
    # RU: Обернуть строку Limbo как UTF-8 с длиной.
    text: fn(value: string): ref Value;

    # EN: Return the exact encoded variant. No I/O; raise for invalid shapes.
    # RU: Вернуть точный кодированный вариант. Некорректная форма даёт исключение.
    encode: fn(value: ref Value): array of byte;

    # EN: Decode one counted scalar or graph. Returns owned data; reject reference tags.
    # RU: Декодировать скаляр или граф с длиной. Результат во владении, теги ссылок отклоняются.
    decode: fn(frame: array of byte): ref Value;

};
