# Copyright (c) 2026 bhgv
# EN: Exercise value snapshots, graph updates and leased channels on one checked Dis module.
# RU: Проверить снимки значений, изменения графа и аренды каналов на одном модуле Dis.
implement Library;

Library: module {
    Wide: adt {
        value: big; # Large value copied back to the original AS ref.
    };
    wide: fn(words: array of string, values: array of big): (array of string, array of big);
    patch: fn(box: ref Wide, next: big): ref Wide;
    widepipe: fn(source: chan of array of big): chan of array of big;
};


# EN: Check copied string and big arrays, then change only the receiver's copies.
# RU: Проверить копии массивов string и big, затем изменить только копии получателя.
wide(words: array of string, values: array of big): (array of string, array of big)
{
    if (len words != 2 || words[0] != "9223372036854775807" || words[1] != "text" ||
        len values != 4 || values[0] != big "9223372036854775807" || values[1] != big "-9223372036854775808" ||
        values[2] != big 42 || values[3] != big "-9007199254740992") {
        raise "wide value snapshot differs";
    }
    words[1] = "copy";
    values[0] -= big 1;
    return (words, values);
}


# EN: Mutate a local ref and return the same identity for staged AS copy-back.
# RU: Изменить локальную ссылку и вернуть ту же идентичность для обратной записи AS.
patch(box: ref Wide, next: big): ref Wide
{
    if (box == nil || box.value != big "-9223372036854775808") {
        raise "wide reference differs";
    }
    box.value = next;
    return box;
}


# EN: Receive an array from the lent channel and emit its adjusted copy on a leased result.
# RU: Принять массив с переданного канала и отправить изменённую копию в арендованный результат.
widepipe(source: chan of array of big): chan of array of big
{
    values := <-source;
    if (len values != 2 || values[0] != big "9223372036854775807" || values[1] != big "-9223372036854775808") {
        raise "wide channel element differs";
    }
    values[0] -= big 1;
    values[1] += big 1;
    result := chan[1] of array of big;
    result <-= values;
    return result;
}
