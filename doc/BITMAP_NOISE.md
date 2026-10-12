# BitmapData.noise

## Русский

v0.60 добавляет noise(randomSeed, low=0, high=255, channelOptions=7,
grayScale=false). Каждый вызов заново инициализирует детерминированный
Park-Miller генератор с multiplier=16807, modulus=2147483647. Random seed
приводится к signed 32-bit; при seed≤0 начальное значение равно 1−seed.
Для INT_MIN это вычисляется без signed overflow. Schrage reduction использует
только стандартные 32-битные long/unsigned long: ни long long, ни uint64_t
не требуются. Math.random state не читается и не изменяется.

Генератор обходится row-major. В color режиме маски red=1, green=2, blue=4,
alpha=8 выбирают очередные samples; выключенные RGB равны нулю, выключенный
alpha равен 255. Grayscale всегда генерирует один общий RGB sample, независимо
от RGB mask; alpha sample остаётся отдельным. Opaque bitmap всё равно
потребляет alpha sample, если mask его выбирает, но сохраняет alpha=255.
В текущей модели zero alpha очищает RGB результата.

low/high приводятся к младшему unsigned байту; при high<low high заменяется
на low. Range включительный, sample=low+(nextState modulo rangeLength).
Числовые NaN/infinity дают integer zero. Gray flag использует AS truth.
Перед seed читаются low/high/channel arguments; после всех callbacks снова
проверяется disposed storage. Без seed вызов сохраняет пиксели; закрытый
receiver возвращает -1. Getter/conversion errors распространяются.

Пиксели создаются в новом полном буфере и публикуются swap только после
завершения. Scratch ограничен memory_limit_bytes, OOM сохраняет старый buffer.
Сохраняется managed object identity: live bitmap-заливки видят новые пиксели.
Callback 1549 append-only и восстанавливается через graph channels без
standard_install в принимающем контексте.

bitmap_noise.cpp проверяет fixed seed vectors, zero/negative/INT_MIN/INT_MAX,
unsigned wrapping, masks/grayscale, range wrapping/clamping, opaque alpha,
неизменность Math RNG, отсутствие seed, quota rollback, disposal из valueOf,
queue/file/loopback TCP и GC. Независимый double modulo oracle проверяет
Schrage последовательность на 17×7 pixels с крайними seeds; integer products
этого oracle точно представимы в IEEE double. bitmap_noise_allocation.cpp
инъецирует new/new[] failures, сверяет прежние pixels и allocation balance
после уничтожения VM. Профили: headless, AGG, Cairo, portable, heap/global
ASan+UBSan; stack ASan и leak scanning отключены в текущем окружении.

Открыто: полная историческая Flash coercion/getter-order/version сверка,
точная premultiplied quantization при частичной прозрачности, Perlin noise,
pixelDissolve, остальные filters/text/media. Этап 4 не завершён. Fixed vectors
подтверждают генератор и текущую storage model, а не полную Flash-совместимость.

## English

v0.60 adds deterministic BitmapData.noise with independent Park-Miller state
(multiplier 16807, modulus 2147483647). Signed seeds ≤0 normalize to 1−seed
without overflowing INT_MIN. Schrage reduction keeps all intermediate products
within signed 32-bit long; no 64-bit integer extension is required. Math.random
state is unaffected.

Pixels consume selected RGB/alpha samples in row-major order. Disabled RGB is
zero, default alpha is 255. Grayscale generates one shared RGB sample regardless
of RGB mask and a separate alpha sample when selected. Opaque targets still
consume selected alpha samples but store 255. Current zero-alpha output clears RGB.
Bounds wrap to bytes, high is raised to low if necessary and samples include both
bounds. NaN/infinite numeric conversions yield integer zero. Missing seed leaves
pixels unchanged; disposed storage returns -1 and conversion errors propagate.

A complete unpublished buffer precedes atomic swap, preserving object identity,
live fills and OOM rollback within the configured scratch budget. Stable callback
1549 survives queue/file/network graph transport without local standard installation.
Tests cover fixed and extreme seed vectors, an independent exact-double modulo
oracle, masks/grayscale, ranges, opacity, Math RNG independence, quotas, getter
disposal, all three transports/GC and injected allocation rollback/balance.
Headless, AGG, Cairo, portable and heap/global ASan+UBSan are exercised; stack
ASan/leak scanning are disabled. Historical Flash coercion/version conformance,
partial-alpha premultiplied quantization, Perlin/pixel dissolve and remaining
filters/text/media keep roadmap stage 4 open.
