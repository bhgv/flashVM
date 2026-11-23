# Copyright (c) 2026 bhgv
# EN: Mutate original call arguments and test a failed remote operation.
# RU: Изменение параметров ref и проверка отказа удалённой операции.
implement Refs;
include "refs.m";


# EN: Observe alias identity and optionally change one or two distinct cells.
# RU: Проверить тождество алиасов и при необходимости изменить один или два объекта.
change(text: string, left, right, other: ref Cell): string
{
    if (left != right || other == left) {
        raise "incorrect identity";
    }
    if (text != "noop") {
        left.text = text;
    }
    if (text == "both") {
        other.text = text;
    }
    if (text == "fail") {
        raise "discard this ref update";
    }
    return right.text;
}


# EN: Mutate the local copy and then raise through the same ABI.
# RU: Изменить локальную копию и завершить исключением через тот же ABI.
reject(text: string, left, right, other: ref Cell): string
{
    left.text = text;
    raise "discard changes";
}
