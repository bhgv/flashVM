/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/text_layout/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/text_layout.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_data.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../example_support.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(int argc, char **argv)
/* Начинаем блок инструкций или описание объекта. */
{
/* Выбираем ветку компиляции для указанной конфигурации. */
    unsigned int flags = 0; /* Keep legacy native shaping unless the caller selects a feature. */
    int tabbed = 0; /* Explicit custom tab table mode keeps borrowed storage local. */
    int indented = 0; /* Optional first-line mode selects positive or hanging indentation. */
    int justified = 0; /* Optional LTR paragraph demonstration keeps the same owned-layout API. */
    int underline = 0; /* Optional line decoration is separate from glyph shaping. */
    int ligatures = 0; /* Select a short fi probe for explicit or tracking-driven decomposition. */
    double spacing = 0; /* Zero preserves all legacy shaping, including complex clusters. */
    if (argc == 2 && !strcmp(argv[1], "--no-kerning")) flags = FVM_FONT_SHAPE_NO_KERNING; /* Disable pair kerning only. */
    else if (argc == 2 && !strcmp(argv[1], "--tracking")) { spacing = 2.5; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Show fractional positive spacing on simple AV. */
    else if (argc == 2 && !strcmp(argv[1], "--condensed")) { spacing = -2; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Condense the same characters with a negative interval. */
    else if (argc == 2 && !strcmp(argv[1], "--underline")) underline = 1; /* Add font-defined bars without changing text metrics. */
    else if (argc == 2 && !strcmp(argv[1], "--no-ligatures")) { ligatures = 1; flags = FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES; } /* Decompose fi without adding tracking. */
    else if (argc == 2 && !strcmp(argv[1], "--ligature-tracking")) { ligatures = 1; spacing = 1; } /* Nonzero tracking requests decomposition automatically. */
    else if (argc == 2 && !strcmp(argv[1], "--justify")) { justified = 1; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Expand only internal ASCII spaces on wrapped nonfinal lines. */
    else if (argc == 2 && !strcmp(argv[1], "--indent")) { indented = 1; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Offset only each paragraph first line by 30px. */
    else if (argc == 2 && !strcmp(argv[1], "--hanging-indent")) { indented = -1; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Allow a native first-line origin at x=-18. */
    else if (argc == 2 && !strcmp(argv[1], "--tabs")) { tabbed = 1; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Use explicit 40/80px tab stops. */
    else if (argc == 2 && !strcmp(argv[1], "--default-tabs")) { tabbed = 2; flags = FVM_FONT_SHAPE_NO_KERNING; } /* Derive a repeating interval from four real average widths. */
    else if (argc != 1) { fputs("usage: text_layout [--no-kerning|--tracking|--condensed|--underline|--no-ligatures|--ligature-tracking|--justify|--indent|--hanging-indent|--tabs|--default-tabs]\n", stderr); return 1; } /* Reject unknown options before allocation. */
#ifdef FVM_HAS_FONTS
    { /* Keep all C89 declarations before the font-enabled block's statements. */
    /* Сохраняем вычисленное значение: const char text[]. */
    const char *text = tabbed ? "A\tV\tA" : justified || indented ? "A V A V" : ligatures ? "fi" : spacing != 0 ? "AV" : "AV fi \xd7\x90\xd7\x91\n\xd8\xa8\xd8\xa8\xd8\xa8"; /* Tracking decomposes optional fi while retaining unambiguous per-character placement. */
    /* Сохраняем вычисленное значение: FVM_Font *font. */
    FVM_Font *font = 0;
    /* Сохраняем вычисленное значение: FVM_TextLayout *layout. */
    FVM_TextLayout *layout = 0;
    /* Сохраняем вычисленное значение: FVM_TextLayoutOptions options. */
    FVM_TextLayoutOptions options = {120, 48, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_TextLayoutMetrics metrics;
    double tab_stops[] = {40, 80}; /* Native creation borrows these two initialized positions only during the call. */
    double period = 0; /* Copy the default interval before releasing the mutable font. */
    FVM_Result created; /* Retain the selected creation call result for ordinary error handling. */
    FVM_FontUnderlineMetrics decoration = {0, 0}; /* Copy the font metrics before destroying the face. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_TextLayoutLine line;
    /* Сохраняем вычисленное значение: FVM_Path *path. */
    FVM_Path *path = 0, *letter = 0;
    /* Сохраняем вычисленное значение: FVM_Affine skew. */
    FVM_Affine skew = {1, 0, 0.2, 1, 0, 0};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned char pixels[192 * 160 * 4];
    /* Сохраняем вычисленное значение: FVM_RasterSurface surface. */
    FVM_RasterSurface surface = {0, 192, 160, 768};
    /* Сохраняем вычисленное значение: FVM_RasterColor color. */
    FVM_RasterColor color = {30, 90, 220, 255};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    size_t i;
    /* Сохраняем вычисленное значение: surface.pixels. */
    surface.pixels = pixels;
    if (justified) /* Keep the analytical paragraph at a fixed 90-pixel inner width. */
    { /* All declarations above precede this C89 configuration block. */
        options.max_width = 90; /* Native justification fills this width on eligible nonfinal lines. */
        options.direction = FVM_TEXT_LTR; /* This bounded policy requires a left-to-right paragraph. */
        options.alignment = FVM_TEXT_ALIGN_JUSTIFY; /* Select the new native enum without changing structure layout. */
    } /* Font/layout/path ownership remains the ordinary tutorial contract. */
    if (indented) /* Reserve a different width on the first visual line only. */
    { /* The native API accepts signed offsets without field margin policy. */
        options.max_width = 102; /* Positive mode leaves 72px on the first line. */
        options.direction = FVM_TEXT_LTR; /* Nonzero indentation requires bounded LTR paragraphs. */
        options.alignment = FVM_TEXT_ALIGN_LEFT; /* Keep intrinsic positioning directly observable. */
    } /* Subsequent lines retain the full 102px width. */
    if (tabbed) /* Keep stops relative to an unaligned, unlimited paragraph content origin. */
    { /* Tab paragraphs currently require bounded LTR support. */
        options.max_width = 0; /* Observe both custom positions without wrapping. */
        options.direction = FVM_TEXT_LTR; /* No mixed bidi paragraph is claimed. */
        options.alignment = FVM_TEXT_ALIGN_LEFT; /* Leave anchor positions directly visible. */
    } /* Existing explicit natural line height remains unchanged. */
    /* Вызываем fvm_font_read: загружаем шрифтовые байты в собственный дескриптор.
     * fvm_test_font — байты шрифтового файла.
     * sizeof(fvm_test_font) — размер входного текста или ресурса в байтах.
     * 0 — индекс шрифтового лица в коллекции.
     * 40 — размер шрифта в пикселях.
     * &font — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail(
            /* Продолжаем условие проверки или аргументы диагностического сообщения. */
            "fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK", __FILE__,
            /* Передаём имя исходного файла и номер строки неуспешной проверки. */
            __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_text_layout_create_spaced: разбиваем текст на строки и размещаем глифы.
     * font — шрифтовой дескриптор.
     * text — текст UTF-8 для разбивки на строки и размещения глифов.
     * strlen(text) — размер входного текста или ресурса в байтах.
     * &options — параметры переносов, направления и выравнивания текста.
     * flags — явный выбор парного кернинга, одинаковый для измерения и вывода.
     * spacing — дополнительные пиксели после каждого поддержанного символа.
     * &layout — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (tabbed == 2) /* Read a declared font metric instead of guessing a tab interval. */
    { /* Explicit errors keep the tutorial honest for fonts without OS/2 metrics. */
        if (fvm_font_average_width(font, &period) != FVM_OK) fvm_example_fail("font average width", __FILE__, __LINE__); /* Borrow font synchronously and copy the positive scalar. */
        period *= 4; /* AS2 defaults use four average character widths. */
    } /* The interval no longer depends on a borrowed pointer. */
    created = tabbed == 2 ? fvm_text_layout_create_periodic(font, text, strlen(text), &options, flags, spacing, 0, period, &layout) /* Return independent repeating-tab geometry without a table. */
                     : tabbed ? fvm_text_layout_create_tabbed(font, text, strlen(text), &options, flags, spacing, 0, tab_stops, 2, &layout) /* Copy tab positions into independently owned layout geometry. */
                     : indented ? fvm_text_layout_create_indented(font, text, strlen(text), &options, flags, spacing, indented > 0 ? 30 : -18, &layout) /* Borrow all inputs, receive an independent owned layout. */
                       : fvm_text_layout_create_spaced(font, text, strlen(text), &options, flags, spacing, &layout); /* Existing modes keep their zero-indent call. */
    if (!(created == FVM_OK)) /* Both creation APIs clear output on failure. */
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail(
            /* Продолжаем условие проверки или аргументы диагностического сообщения. */
            "fvm_text_layout_create_spaced(font, text, strlen(text), &options, flags, spacing, &layout) == FVM_OK",
            /* Передаём имя исходного файла и номер строки неуспешной проверки. */
            __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    if (tabbed == 1) tab_stops[0] = tab_stops[1] = 0; /* Borrowed table mutation cannot change already placed glyphs. */
    if (underline && fvm_font_underline_metrics(font, &decoration) != FVM_OK) /* Borrow the face; publish copied y-down pixel metrics. */
        fvm_example_fail("fvm_font_underline_metrics", __FILE__, __LINE__); /* Stop explicitly if the font lacks usable decoration data. */
    /* Вызываем fvm_font_destroy: освобождаем шрифтовой дескриптор.
     * font — шрифтовой дескриптор. */
    fvm_font_destroy(font);
    /* Вызываем fvm_text_layout_metrics: читаем размеры текстового блока.
     * layout — ранее созданная текстовая раскладка.
     * &metrics — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_text_layout_metrics(layout, &metrics) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_text_layout_metrics(layout, &metrics) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Выводим результат в стандартный поток вывода. */
    printf("lines=%lu glyphs=%lu advance=%.2f height=%.2f\n", (unsigned long)metrics.line_count,
           /* Завершаем выражение с указанным значением или аргументами. */
           (unsigned long)metrics.glyph_count, metrics.advance_width, metrics.height);
    /* Организуем повторение операций до выполнения условия завершения. */
    for (i = 0; i < metrics.line_count; ++i)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Вызываем fvm_text_layout_line: читаем параметры выбранной строки.
         * layout — ранее созданная текстовая раскладка.
         * i — индекс выбираемого элемента.
         * &line — адрес или буфер для результата.
         * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
        if (!(fvm_text_layout_line(layout, i, &line) == FVM_OK))
        /* Начинаем обработку неуспешного вызова или неверного результата. */
        {
            /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
            fvm_example_fail("fvm_text_layout_line(layout, i, &line) == FVM_OK", __FILE__,
                             /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                             __LINE__);
            /* Завершаем ветку ошибки. */
        }
        /* Выводим результат в стандартный поток вывода. */
        printf("line %lu: bytes [%lu,%lu), baseline %.2f, direction %s\n", (unsigned long)i,
               /* Продолжаем объявление или выражение с указанными параметрами. */
               (unsigned long)line.byte_start, (unsigned long)line.byte_end, line.baseline,
               /* Завершаем выражение с указанным значением или аргументами. */
               line.direction == FVM_TEXT_RTL ? "RTL" : "LTR");
        /* Завершаем текущий блок или описание объекта. */
    }
    if (justified) /* Observe expanded and final natural advances after the font was destroyed. */
    { /* The layout owns the copied metrics and glyph outlines. */
        double first, last; /* Copy scalar observations before reusing the line result. */
        if (fvm_text_layout_line(layout, 0, &line) != FVM_OK) fvm_example_fail("read justified first line", __FILE__, __LINE__); /* Read an existing zero-based line index. */
        first = line.advance; /* The nonfinal line expands to exactly 90 pixels. */
        if (fvm_text_layout_line(layout, metrics.line_count - 1, &line) != FVM_OK) fvm_example_fail("read final line", __FILE__, __LINE__); /* The final paragraph line is not stretched. */
        last = line.advance; /* Two 24-pixel letters and a 12-pixel internal space give 60 pixels. */
        printf("justify first=%.2f last=%.2f\n", first, last); /* The executable checker verifies both independent oracle values. */
    } /* Existing path export/rasterization continues below using the same owned layout. */
    if (indented) /* Inspect source-owned line geometry after releasing the font. */
    { /* Copy observations because each line query overwrites its output. */
        double first, next; /* Signed x is independent of glyph advance. */
        if (fvm_text_layout_line(layout, 0, &line) != FVM_OK) fvm_example_fail("indented first line", __FILE__, __LINE__); /* Query the first paragraph line. */
        first = line.x; /* Expected 30 or -18 pixels. */
        if (fvm_text_layout_line(layout, 1, &line) != FVM_OK) fvm_example_fail("indented continuation", __FILE__, __LINE__); /* Query the wrapped continuation. */
        next = line.x; /* Continuations have no first-line offset. */
        printf("indent first_x=%.2f next_x=%.2f\n", first, next); /* The executable checker verifies both signed modes. */
    } /* Existing separately owned path export and rasterization follow. */
    if (tabbed) /* Inspect placed word origins after table mutation and font release. */
    { /* Both glyph queries borrow the immutable layout. */
        FVM_TextLayoutGlyph glyph; /* Copy each shaped record rather than retaining an internal pointer. */
        double first, second; /* Save the two actual post-tab word positions. */
        if (fvm_text_layout_glyph(layout, 2, &glyph) != FVM_OK) fvm_example_fail("first tab anchor", __FILE__, __LINE__); /* Tab records retain their own source glyph slots. */
        first = glyph.x; /* Copy the first explicit or font-derived word origin. */
        if (fvm_text_layout_glyph(layout, 4, &glyph) != FVM_OK) fvm_example_fail("second tab anchor", __FILE__, __LINE__); /* Read the final word independently. */
        second = glyph.x; /* Copy the second explicit or font-derived word origin. */
        printf("%s first_stop=%.2f second_stop=%.2f\n", tabbed == 2 ? "default-tabs" : "tabs", first, second); /* Default mode gives 87.50/175.00; explicit mode retains 40.00/80.00. */
    } /* Empty tab outlines and real word paths share the same owned layout. */
    /* Вызываем fvm_text_layout_path: получаем контуры всего текстового блока.
     * layout — ранее созданная текстовая раскладка.
     * &path — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_text_layout_path(layout, &path) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_text_layout_path(layout, &path) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    if (underline) /* Add separate line contours to the owned whole-text path. */
    { /* Keep all C89 block declarations before statements. */
        size_t original = fvm_path_count(path); /* Observe glyph-only commands without allocation. */
        FVM_Affine identity = {1, 0, 0, 1, 0, 0}; /* The layout already placed each line. */
        for (i = 0; i < metrics.line_count; ++i) /* Visit each placed line, including empty lines. */
        { /* Own one temporary bar per iteration. */
            FVM_Path *bar = 0; /* The helper returns independent path storage. */
            FVM_Result status = fvm_text_layout_line_underline_path(layout, i, decoration.center_y, decoration.thickness, &bar); /* Borrow the layout; empty lines yield empty paths. */
            if (status == FVM_OK) status = fvm_path_append(path, bar, &identity); /* Copy the placed rectangle into the whole-text path. */
            fvm_path_destroy(bar); /* Release the temporary path even if append failed. */
            if (status != FVM_OK) fvm_example_fail("line underline export", __FILE__, __LINE__); /* Do not continue after partial geometry failure. */
        } /* All temporary bar borrows and ownership end here. */
        printf("underline center=%.2f thickness=%.2f commands=%lu\n", decoration.center_y, decoration.thickness, (unsigned long)(fvm_path_count(path) - original)); /* Expose the analytical fixture geometry. */
    } /* The combined owned path no longer depends on bar storage. */
    /* Вызываем fvm_text_layout_glyph_path: получаем векторный контур выбранного глифа.
     * layout — ранее созданная текстовая раскладка.
     * 0 — индекс выбираемого элемента.
     * &letter — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_text_layout_glyph_path(layout, 0, &letter) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_text_layout_glyph_path(layout, 0, &letter) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_text_layout_destroy: освобождаем текстовую раскладку.
     * layout — ранее созданная текстовая раскладка. */
    fvm_text_layout_destroy(layout);
    /* Both the block and its first glyph are now independent editable vectors. */
    /* Вызываем fvm_path_transform: применяем аффинное преобразование к пути.
     * letter — векторный путь.
     * &skew — матрица аффинного преобразования или дескриптор мутекса.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_path_transform(letter, &skew) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_path_transform(letter, &skew) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Инициализируем область памяти заданным байтом. */
    memset(pixels, 0, sizeof(pixels));
/* Выбираем ветку компиляции для указанной конфигурации. */
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    /* Вызываем fvm_path_draw: отрисовываем путь с указанной заливкой и границей.
     * path — векторный путь.
     * &surface — строка или используемый дескриптор.
     * color — цвет заливки/границы в RGBA.
     * 0 — ширина границы или текстового блока в пикселях.
     * 0 — 1 — правило чётности заливки, 0 — правило ненулевого обхода.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_path_draw(path, &surface, color, 0, 0) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_path_draw(path, &surface, color, 0, 0) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_path_draw: отрисовываем путь с указанной заливкой и границей.
     * letter — векторный путь.
     * &surface — строка или используемый дескриптор.
     * color — цвет заливки/границы в RGBA.
     * 1 — ширина границы или текстового блока в пикселях.
     * 0 — 1 — правило чётности заливки, 0 — правило ненулевого обхода.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_path_draw(letter, &surface, color, 1, 0) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_path_draw(letter, &surface, color, 1, 0) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    if (underline) /* Probe an interior pixel in the first bar, below all glyph outlines. */
    { /* The 40px fixture puts the first bar at y=[39,41], spanning x=[0,96.8]. */
        unsigned int alpha = pixels[40 * surface.stride + 7]; /* Read RGBA alpha at pixel (1,40), safely inside the caller buffer. */
        if (alpha != color.a) fvm_example_fail("underline raster pixel", __FILE__, __LINE__); /* Require actual opaque bar coverage. */
        printf("underline probe_alpha=%u\n", alpha); /* Let the executable tutorial checker inspect the independent pixel oracle. */
    } /* Pixel storage remains owned by this stack frame. */
/* Начинаем альтернативную ветку компиляции. */
#else
    /* Вызываем fvm_path_draw: отрисовываем путь с указанной заливкой и границей.
     * path — векторный путь.
     * &surface — строка или используемый дескриптор.
     * color — цвет заливки/границы в RGBA.
     * 0 — ширина границы или текстового блока в пикселях.
     * 0 — 1 — правило чётности заливки, 0 — правило ненулевого обхода.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_path_draw(path, &surface, color, 0, 0) == FVM_UNSUPPORTED))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_path_draw(path, &surface, color, 0, 0) == FVM_UNSUPPORTED", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
/* Завершаем условный блок компиляции. */
#endif
    /* Вызываем fvm_path_destroy: освобождаем векторный путь.
     * letter — векторный путь. */
    fvm_path_destroy(letter);
    /* Вызываем fvm_path_destroy: освобождаем векторный путь.
     * path — векторный путь. */
    fvm_path_destroy(path);
/* Начинаем альтернативную ветку компиляции. */
    } /* Release the font-enabled declaration scope after native cleanup. */
#else
    /* Выводим строку с завершающим переводом строки. */
    (void)tabbed; /* Parsing remains available without fonts. */
    (void)indented; /* Keep both modes parseable in a NO_FONTS build. */
    (void)justified; /* Mode parsing remains available in a NO_FONTS build. */
    (void)underline; /* The option still parses without a font backend. */
    (void)ligatures; /* The short probe remains parseable without fonts. */
    (void)spacing; /* Numeric option parsing does not require a font handle. */
    (void)flags; /* Feature parsing remains available when native fonts are disabled. */
    puts("Text layout requires FONTS in DEFS.txt.");
/* Завершаем условный блок компиляции. */
#endif
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
