# Copyright (c) 2026 bhgv
# EN: Return real DisVM buffered channels and one repeated reference.
# RU: Возвратить настоящие буферизованные каналы DisVM и одну повторную ссылку.
implement Channels;
include "channel.m";


# EN: Seed two independently usable channels on the same issuing fid.
# RU: Заполнить два независимых канала на одном выдающем fid.
ports(value: int): (chan of int, chan of int, chan of string)
{
    integers := chan[1] of int;
    text := chan[1] of string;
    integers <-= value;
    text <-= "a\0б";
    return (integers, integers, text);
}


# EN: Return nil without creating or pinning a channel.
# RU: Возвратить nil без создания и удержания канала.
empty(): chan of int
{
    return nil;
}
