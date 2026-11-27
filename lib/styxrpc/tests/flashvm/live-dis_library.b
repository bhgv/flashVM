# Copyright (c) 2026 bhgv
# EN: Expose scalar and managed calls on one ordinary Dis library.
# RU: Предоставить скалярные и управляемые вызовы обычной библиотеки Dis.
implement Library;

Library: module {
    increment: fn(value: int): int;
    current: fn(value: int): int;
    label: fn(value: string): string;
    bounce: fn(source: chan of int): chan of int;
};

total := 5; # Initial state per loaded instance.


# EN: Update the instance total by value. / RU: Изменить итог экземпляра на value.
increment(value: int): int
{
    total += value;
    return total;
}


# EN: Read the same total with offset value. / RU: Прочитать итог со смещением value.
current(value: int): int
{
    return total + value;
}


# EN: Return a managed string from the owner. / RU: Вернуть управляемую строку владельца.
label(value: string): string
{
    return value + "!";
}


# EN: Receive the caller's value and return it through an independent live channel.
# RU: Принять значение клиента и вернуть его через независимый живой канал.
bounce(source: chan of int): chan of int
{
    value := <-source;
    result := chan[1] of int;
    result <-= value + 1;
    return result;
}
