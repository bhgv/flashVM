/* Сравниваем RGB и linearRGB по реальным пикселям заливки и обводки. */
static FVM_Result gradient_linear_rgb_demo(const FVM_Path *path, FVM_RasterSurface *surface)
{ /* Начинаем синхронный helper; путь и буфер принадлежат main. */
    FVM_Gradient gradient; /* Создаём локальное описание с прежним ABI layout. */
    FVM_GradientStop stops[2]; /* Удерживаем два stops на стеке до завершения всех вызовов. */
    FVM_Result result; /* Сохраняем исходный статус без assert в учебном коде. */
    unsigned int stroke; /* Выбираем сначала заливку, затем обводку. */
    unsigned int interpolation; /* Перебираем unsigned RGB=0 и LINEAR_RGB=1. */
    unsigned int row; /* Выбираем строку полностью покрытого пикселя. */
    const unsigned char *pixel; /* Заимствуем адрес одного пикселя только для printf. */
    result = fvm_gradient_init(&gradient); /* Инициализируем матрицу и нейтральный color transform. */
    if (result != FVM_OK) return result; /* Передаём неожиданную ошибку инициализации вызывающему. */
    stops[0].ratio = 0; /* Начинаем рампу с полностью прозрачного чёрного. */
    stops[0].color.r = stops[0].color.g = stops[0].color.b = stops[0].color.a = 0; /* Задаём прямые RGBA bytes первого stop. */
    stops[1].ratio = 1; /* Заканчиваем рампу полностью непрозрачным белым. */
    stops[1].color.r = stops[1].color.g = stops[1].color.b = stops[1].color.a = 255; /* Задаём прямые RGBA bytes второго stop. */
    gradient.stops = stops; /* Заимствуем массив до возврата каждого синхронного вызова. */
    gradient.count = 2; /* Сообщаем точное число доступных stops. */
    gradient.matrix.xx = gradient.matrix.yy = 8; /* Растягиваем нормализованные оси до шестнадцати пикселей. */
    gradient.matrix.x0 = gradient.matrix.y0 = 16.5; /* Середина рампы проходит через центр выбранного пикселя. */
    for (stroke = 0; stroke < 2; ++stroke) /* Сравниваем оба вида покрытия независимо. */
    { /* Начинаем одну пару RGB/linearRGB. */
        for (interpolation = 0; interpolation < 2; ++interpolation) /* Не меняем stops между цветовыми политиками. */
        { /* Начинаем отрисовку на прозрачный буфер. */
            memset(surface->pixels, 0, surface->stride * surface->height); /* Удаляем результат предыдущего вызова. */
            if (stroke) /* Для обводки используем положительную ширину четыре пикселя. */
                result = fvm_path_stroke_gradient_interpolated(path, surface, &gradient, 4, FVM_GRADIENT_SPREAD_PAD, interpolation); /* Выбираем цветовую политику без изменения layout градиента. */
            else /* Для заливки выбираем прежнее правило nonzero. */
                result = fvm_path_draw_gradient_interpolated(path, surface, &gradient, 0, FVM_GRADIENT_SPREAD_PAD, interpolation); /* Alpha остаётся прямым линейным каналом в обоих режимах. */
            if (result != FVM_OK) /* Не читаем пиксели после ошибки подготовки или backend. */
            { /* Начинаем диагностику; main по-прежнему владеет путём. */
                fprintf(stderr, "linearRGB drawing failed: %d\n", (int)result); /* Печатаем неизменённый статус API. */
                return result; /* Передаём ошибку для общей очистки main. */
            } /* Завершаем обработку ошибки. */
            row = stroke ? 4 : 16; /* Выбираем верхнюю сторону обводки или внутренность заливки. */
            pixel = surface->pixels + row * surface->stride + 16 * 4; /* Находим середину рампы в принадлежащем main буфере. */
            printf("linear %s %s: %u %u %u %u\n", stroke ? "stroke" : "fill", interpolation ? "linearRGB" : "RGB", /* Подписываем покрытие и цветовую политику. */
                   (unsigned int)pixel[0], (unsigned int)pixel[1], (unsigned int)pixel[2], (unsigned int)pixel[3]); /* Читаем straight RGBA: цвет 128/188 и неизменную alpha 128. */
        } /* Заканчиваем пару цветовых политик. */
    } /* Заканчиваем заливку и обводку без сохранения указателей. */
    return FVM_OK; /* Сообщаем успех; путь освободит main. */
} /* Завершаем helper; локальные stops больше не используются. */
