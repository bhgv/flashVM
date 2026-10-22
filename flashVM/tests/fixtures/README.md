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

geometry_bold.ttf, geometry_italic.ttf and geometry_bold_italic.ttf (with matching
font_*_data.h headers) are separate original CC0 faces. At 40px, AV advances are
56, 52 and 60 pixels respectively, versus 48 regular with kerning disabled.
Their AV contours are embedded wider/slanted outlines, not runtime simulation.
Other glyphs remain simple probe geometry; this is no typographic quality oracle.
The generator reproduces all six faces; builds do not require fontTools.

Отдельные CC0 bold/italic/bold-italic лица при 40px дают AV advance 56/52/60
против 48 у regular без кернинга. Широкие/наклонные AV-контуры находятся в самих
лицах, не синтезируются во время исполнения. Остальные glyphs — простые probes;
это не проверка качества типографики. Генератор воспроизводит все шесть лиц.
