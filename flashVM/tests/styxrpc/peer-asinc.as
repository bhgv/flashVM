// Copyright (c) 2026 bhgv
// EN: Compute one value in an isolated worker. The player owner never runs this body.
// RU: Вычислить значение в отдельном потоке. Владелец плеера не выполняет это тело.
asinc function compute() {
    var checksum = 40;
    return checksum + 2;
}
