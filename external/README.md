# Bundled dependencies

New dependencies live in `external/`, as requested for the SWF 8 development
phase. Existing `extern/mini_regex` remains at its historical location.

- zlib 1.3.1: complete upstream source, built offline; see `zlib/README.md`.
- AGG: pinned portable core from ghaerr/agg-2.6; see `agg/README.md` for
  provenance, license and strict C++98 patches.
- Cairo: not bundled/built. Cairo 1.18.4's upstream Meson project requests
  C11 and C++11. Its pixman and font/image dependencies need a separate
  compatibility and build audit before inclusion under the language contract.

`external/` содержит новые зависимости; старый `extern/mini_regex` сохранён.
Исходники zlib и ядра AGG входят в архив и собираются без скачиваний.
Cairo пока не включён: требуется отдельная проверка совместимости со строгим
C89/C++98 и включение транзитивных зависимостей, включая pixman.
