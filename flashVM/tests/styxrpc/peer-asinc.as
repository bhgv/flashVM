// Copyright (c) 2026 bhgv
// EN: Mutate a copied input, then read it through an alias in the isolated worker.
// RU: Изменить копию входа и прочитать её через вторую ссылку в потоке.
asinc function compute(input, alias, delta) {
    input.score = input.score + delta;
    return alias.score;
}
