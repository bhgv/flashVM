# Copyright (c) 2026 bhgv
# EN: Original image with two shared-state methods and one unsupported wire shape.
# RU: Исходный образ с двумя методами на общих данных и одной неподдерживаемой схемой.
implement Original;

Original: module {
    add: fn(value: int): int;
    current: fn(ignored: int): int;
    label: fn(value: string): string;
    ports: fn(value: int): chan of int;
    skip: fn(values: list of int): list of int;
};

total := 5; # Shared data initialized once in each independent load.


# EN: Mutate this instance by value and return the new total.
# RU: Изменить этот экземпляр на value и вернуть новый итог.
add(value: int): int
{
    total += value;
    return total;
}


# EN: Read the same module data through another export.
# RU: Прочитать данные того же модуля через другой экспорт.
current(ignored: int): int
{
    return total + ignored;
}


# EN: Echo a list, a shape outside the scalar owner wire contract.
# RU: Вернуть список, схема которого не поддерживается протоколом владельца.
skip(values: list of int): list of int
{
    return values;
}


# EN: Return a shape requiring the managed string gateway.
# RU: Вернуть значение через управляемый строковый шлюз.
label(value: string): string
{
    return value + "!";
}


# EN: Provide an export with a live channel result in its compiled shape.
# RU: Предоставить экспорт с живым каналом в компилированной схеме результата.
ports(value: int): chan of int
{
    result := chan[1] of int;
    result <-= value;
    return result;
}
