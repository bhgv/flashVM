# Copyright (c) 2026 bhgv
# EN: Dis implementations for sparse nested edits and shared returned identities.
# RU: Реализации Dis для разреженных вложенных изменений и общих возвращаемых ссылок.
implement Graphs;
include "graph.m";


# EN: Update a child and return the same original identity.
# RU: Изменить вложенный объект и вернуть прежний идентификатор.
nested(box: ref Box): ref Leaf
{
    box.inner.count++;
    return box.inner;
}


# EN: Publish a fresh identity in both result and an existing parent's field.
# RU: Выдать новый идентификатор в результате и поле исходного объекта.
replace(box: ref Box, value: int): ref Leaf
{
    box.inner = ref Leaf(value);
    return box.inner;
}


# EN: Return a fresh ref from value-only arguments.
# RU: Вернуть новый ref из аргумента по значению.
make(value: int): ref Leaf
{
    return ref Leaf(value);
}


# EN: Mutate before failure to verify that the result and update are discarded.
# RU: Изменить поле перед отказом для проверки сброса результата и изменений.
fail(box: ref Box): ref Leaf
{
    box.inner.count++;
    raise "discard graph changes";
}


# EN: Edit an array element whose ID lies beyond the ordinary ref limit.
# RU: Изменить элемент массива с ID за пределами обычного профиля ref.
fan(values: array of ref Leaf): ref Leaf
{
    values[12].count++;
    return values[0];
}


# EN: Replace the integer tuple field and keep the nested identity.
# RU: Заменить целое поле кортежа и сохранить вложенную ссылку.
tupleedit(box: ref TupleBox, value: int): ref Leaf
{
    (unused, child) := box.pair;
    box.pair = (value, child);
    return child;
}
