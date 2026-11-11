# Copyright (c) 2026 bhgv
# EN: Deliberately incompatible export signature for real concurrent link failures.
# RU: Намеренно несовместимая сигнатура экспорта для настоящих ошибок линковки.
BadBurst: module {
    run: fn(value: string): int; # A string argument cannot satisfy Burst.run(int).
};
