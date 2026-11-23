# Copyright (c) 2026 bhgv
# EN: Define a channel that can publish fresh channels across both UniBus directions.
# RU: Объявить канал для выдачи новых каналов в обоих направлениях UniBus.
Channelrecursive: module
{
    open: fn(): (chan of chan of chan of chan of int, chan of int);
};
