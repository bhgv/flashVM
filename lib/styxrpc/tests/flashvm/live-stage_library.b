# Copyright (c) 2026 bhgv
# EN: Compiled exports for typed lists, explicit sparse arrays and copy-back with live channels.
# RU: Скомпилированные экспорты списков, явной разреженности и обратной записи с каналами.
implement Library;
include "bigtable.m";

Library: module {
    Box: adt {
        value: big; # Changed in the same call that issues a channel.
    };
    words: fn(items: list of (string, big)): list of (string, big);
    sparse: fn(values: (int, array of (int, string))): (int, array of (int, string));
    combine: fn(box: ref Box, input: chan of int): (ref Box, chan of big);
    newtable: fn(): ref Bigtable->Handle;
    update: fn(table: ref Bigtable->Handle): chan of big;
};


# EN: Verify list order and native tuple heads, preserving the empty/nil list.
# RU: Сверить порядок списка и нативные кортежи, сохраняя пустой список nil.
words(items: list of (string, big)): list of (string, big)
{
    if (items == nil) {
        return nil;
    }
    if (len items != 2) {
        raise "wrong list length";
    }
    (first, minimum) := hd items;
    (second, maximum) := hd tl items;
    if (first != "☀" || second != "end" ||
        minimum != big "-9223372036854775808" || maximum != big "9223372036854775807") {
        raise "wrong typed list values";
    }
    return (second, maximum - big 1) :: (first, minimum + big 1) :: nil;
}


# EN: Keep the logical length and move one explicitly present slot.
# RU: Сохранить логическую длину и перенести один явно присутствующий слот.
sparse(values: (int, array of (int, string))): (int, array of (int, string))
{
    (length, slots) := values;
    if (length != 12 || len slots != 2) {
        raise "wrong sparse extent";
    }
    (first, left) := slots[0];
    (second, right) := slots[1];
    if (first != 2 || left != "☀" || second != 5 || right != "end") {
        raise "wrong sparse tuple";
    }
    slots[1] = (9, "moved");
    return (length, slots);
}


# EN: Mutate the copied ref, return its identity, and issue an independently leased queue.
# RU: Изменить копию ref, вернуть её идентичность и выдать отдельную аренду очереди.
combine(box: ref Box, input: chan of int): (ref Box, chan of big)
{
    step := <-input;
    if (step != 7 || box == nil || box.value != big "-9223372036854775808") {
        raise "wrong mixed call";
    }
    box.value += big step;
    output := chan[1] of big;
    output <-= box.value;
    return (box, output);
}


# EN: Return a local hybrid table snapshot to be passed alongside a later channel result.
# RU: Вернуть снимок гибридной таблицы для вызова с каналом результата.
newtable(): ref Bigtable->Handle
{
    tables := load Bigtable Bigtable->PATH;
    return tables->create();
}


# EN: Stage a table key patch and a leased channel from the same call.
# RU: Подготовить патч ключа таблицы и аренду канала в одном вызове.
update(table: ref Bigtable->Handle): chan of big
{
    tables := load Bigtable Bigtable->PATH;
    if (tables->settext(table, "wide", big "9223372036854775807") != tables->Ok) {
        raise "cannot update table";
    }
    output := chan[1] of big;
    output <-= big "-9223372036854775808";
    return output;
}
