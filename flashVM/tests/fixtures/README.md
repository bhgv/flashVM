# Font fixture / Тестовый шрифт

geometry.ttf and font_data.h were created for FlashVM from the original simple
geometric contours in scripts/generate_test_font.py. They are dedicated to the
public domain under CC0-1.0. No system/commercial font was copied. They contain
quadratic curves, a hole, a composite glyph, Latin/Hebrew/Arabic mappings,
a fi ligature, AV GPOS kerning and Arabic positional substitutions.
The committed files are sufficient for builds; regeneration needs fontTools.

Шрифт создан для тестов FlashVM из собственных геометрических контуров и
передан в общественное достояние по CC0-1.0. Системные шрифты не копировались.
Сборка не требует fontTools; пакет нужен только для повторного создания данных.
