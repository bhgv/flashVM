// A small valid Limbo module, without dependencies on Inferno headers.
// Небольшой модуль Limbo без зависимостей от заголовков Inferno.
implement Answer;

// Export one function returning a signed 32-bit integer.
// Экспортируем функцию с целым 32-битным результатом.
Answer: module {
    value: fn(): int;
};

// The teaching adapter's SWF harness prints the returned value.
// Учебная SWF-обвязка адаптера печатает возвращённое значение.
value(): int {
    // Wrap at the signed 32-bit boundary / Переполнение по модулю 2^32.
    return 2147483647 + 1;
}
