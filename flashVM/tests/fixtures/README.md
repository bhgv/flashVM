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

geometry_ligatures.ttf / font_ligature_data.h are a separate CC0 OpenType oracle:
standard liga f+i→fi, contextual clig f+f→fi, and required rlig V+A→fi. The latter
two are deliberately synthetic feature-selection probes, not a natural-language
font. The ordinary and mixed-winding fixtures remain byte-for-byte unchanged.
The generator emits all three fonts deterministically; runtime builds use the
committed bytes and do not need Python/fontTools.

Отдельный CC0 geometry_ligatures.ttf проверяет liga f+i→fi, clig f+f→fi и rlig
V+A→fi. Последние два правила намеренно синтетические, для проверки выбора
features; это не шрифт естественного языка. Старые fixtures не изменены.
