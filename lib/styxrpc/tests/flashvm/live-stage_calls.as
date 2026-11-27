// Copyright (c) 2026 bhgv
// EN: Exercise list and sparse values, ref aliases, table updates and an independent channel lease.
// RU: Проверить списки, пропуски, алиасы ref, обновления таблицы и отдельную аренду канала.


// EN: Round-trip two by-value types through compiled Limbo methods.
// RU: Передать два типа по значению и получить их из скомпилированных методов Limbo.
function proveSequences(proxy) {
    var empty = "";
    var first = empty.split("");
    var countFirst = first.push("☀", "-9223372036854775808");
    var second = empty.split("");
    var countSecond = second.push("end", "9223372036854775807");
    var list = empty.split("");
    var countList = list.push(first, second);
    var received = proxy.words(list);
    var start = received[0];
    var end = received[1];
    var missing = proxy.words(empty.split(""));
    var values = empty.split("");
    values[5] = "end";
    values[2] = "☀";
    values.length = 12;
    var packed = vfs.packSparse(values);
    var changed = proxy.sparse(packed);
    var restored = vfs.unpackSparse(changed);
    var hole = restored.hasOwnProperty("5");
    return start[0] + "|" + start[1] + "|" + end[0] + "|" + end[1] + "|" + missing + "|" +
        restored.length + "|" + restored[2] + "|" + restored[9] + "|" + hole + "|" + values[5] + "|";
}
