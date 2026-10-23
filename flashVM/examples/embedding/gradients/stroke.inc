/* Сравниваем три spread-режима обводки; все входные данные остаются у main. */
static FVM_Result gradient_stroke_demo(const FVM_Path *path, FVM_RasterSurface *surface, FVM_Gradient *gradient)
{ /* Начинаем синхронный helper без VM и без устройства вывода. */
    const char *names[3] = {"pad", "reflect", "repeat"}; /* Подписываем соответствующие unsigned selectors 0, 1 и 2. */
    unsigned int mode; /* Объявляем счётчик до исполняемых операторов для C89. */
    FVM_Result result; /* Сохраняем ошибку подготовки или рисования обводки. */
    gradient->matrix.xx = 4; /* Задаём горизонтальную полуширину одного периода в пикселях. */
    gradient->matrix.yy = 4; /* Сохраняем невырожденную вертикальную ось матрицы. */
    gradient->matrix.x0 = 8.5; /* Выравниваем фазы по центрам выбранных пикселей. */
    gradient->matrix.y0 = 16.5; /* Вертикальный центр не меняет линейную цветовую рампу. */
    for (mode = 0; mode < 3; ++mode) /* Рисуем один путь с каждым режимом продолжения. */
    { /* Начинаем независимую отрисовку на очищенную поверхность. */
        memset(surface->pixels, 0, surface->stride * surface->height); /* Очищаем весь принадлежащий main буфер. */
        result = fvm_path_stroke_gradient(path, surface, gradient, 4, mode); /* Заимствуем путь и stops только до возврата; ширина равна четырём пикселям. */
        if (result != FVM_OK) /* Обрабатываем ошибку вместо assert в учебном коде. */
        { /* Начинаем диагностику; main освободит путь при любом результате. */
            fprintf(stderr, "stroke drawing failed: %d\n", (int)result); /* Печатаем исходный статус API. */
            return result; /* Не читаем пиксели после неуспешного рисования. */
        } /* Заканчиваем обработку ошибки этого режима. */
        printf("stroke %s: %u %u %u\n", names[mode], /* Подписываем красные компоненты трёх выборок на верхней стороне. */
               (unsigned int)surface->pixels[4 * (4 * surface->width + 8)], /* Фаза 0.5 даёт половину красного во всех режимах. */
               (unsigned int)surface->pixels[4 * (4 * surface->width + 12)], /* Фаза 1 различает конец рампы и начало repeat. */
               (unsigned int)surface->pixels[4 * (4 * surface->width + 20)]); /* Фаза 2 различает pad и периодические режимы. */
    } /* Завершаем три отрисовки; последняя остаётся в буфере main. */
    return FVM_OK; /* Сообщаем успех без сохранения заимствованных указателей. */
} /* Заканчиваем helper; освобождение пути остаётся у main. */
