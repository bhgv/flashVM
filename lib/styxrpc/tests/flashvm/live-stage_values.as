// Copyright (c) 2026 bhgv
// EN: Check live capabilities and reject use of a captured proxy after replacement.
// RU: Проверить живые возможности и отказ старого прокси после замены.


// EN: Perform one indexed transaction per operation, retaining distinct proxies for nested fields.
// RU: Выполнить отдельный обмен на операцию с независимыми прокси вложенных полей.
function proveLiveVariables() {
    var root = vfs.liveVariable("/live/.var", true);
    var numbers = root.field("numbers");
    var details = root.field("details");
    var scalar = root.field("scalar");
    var name = details.field("name");
    var wide = details.field("wide");
    var magnitude = details.field("magnitude");
    var fields = root.length();
    var length = numbers.length();
    var old = numbers.get(2);
    var changed = numbers.set(0, 42);
    var alias = numbers.get(2);
    var liveInt = scalar.get();
    var oldName = name.get();
    var changedName = name.set("☀day");
    var liveName = name.get();
    var last = 0 - 1;
    var changedWide = wide.set(last, "9223372036854775807");
    var liveWide = wide.get(0);
    var minimum = magnitude.get();
    var assigned = magnitude.set("9223372036854775807");
    var maximum = magnitude.get();
    var magnitudeClosed = magnitude.close();
    var closed = wide.close();
    var nameClosed = name.close();
    var scalarClosed = scalar.close();
    var detailsClosed = details.close();
    var numbersClosed = numbers.close();
    var rootClosed = root.close();
    return fields + "|" + length + "|" + old + "|" + alias + "|" + liveInt + "|" + oldName + "|" +
        liveName.length + "|" + liveName.charAt(1) + "|" + liveWide + "|" + minimum + "|" + maximum + "|" + closed + "|";
}
