/* Сравниваем отрицательный, нулевой и положительный внутренний focal по реальным пикселям. */
static FVM_Result gradient_focal_demo(const FVM_Path *path, FVM_RasterSurface *surface)
{ /* Начинаем синхронный helper; путь и буфер остаются у main. */
    FVM_Gradient gradient; /* Создаём локальное описание с неизменённым ABI layout. */
    FVM_GradientStop stops[2]; /* Локальные stops существуют до возврата всех draw-вызовов. */
    const double focals[3] = {-0.5, 0, 0.5}; /* Выбираем две внутренние позиции и прежний центр. */
    const char *names[3] = {"-0.5", "0", "0.5"}; /* Подписываем измеренные значения без форматирования double. */
    unsigned int stroke; /* Перебираем сначала заливку, затем обводку. */
    unsigned int index; /* Перебираем три focal-позиции в C89-совместимом цикле. */
    unsigned int row; /* Выбираем строку полностью покрытых пикселей. */
    FVM_Result result; /* Обрабатываем исходный статус API без assert. */
    result = fvm_gradient_init(&gradient); /* Устанавливаем единичную матрицу и нейтральный color transform. */
    if (result != FVM_OK) return result; /* Возвращаем неожиданную ошибку инициализации вызывающему. */
    gradient.type = FVM_GRADIENT_RADIAL; /* Ненулевой focal допустим только для радиальной геометрии. */
    stops[0].ratio = 0; /* Помещаем непрозрачный красный цвет в focal-точку. */
    stops[0].color.r = stops[0].color.a = 255; /* Задаём красную компоненту и полную alpha. */
    stops[0].color.g = stops[0].color.b = 0; /* Убираем остальные цветовые компоненты первого stop. */
    stops[1].ratio = 1; /* Помещаем непрозрачный синий цвет на единичную окружность. */
    stops[1].color.b = stops[1].color.a = 255; /* Задаём синюю компоненту и полную alpha. */
    stops[1].color.r = stops[1].color.g = 0; /* Убираем красный и зелёный из второго stop. */
    gradient.stops = stops; /* Заимствуем stops только на время каждого синхронного вызова. */
    gradient.count = 2; /* Сообщаем точное число доступных stops. */
    gradient.matrix.xx = gradient.matrix.yy = 8; /* Задаём радиус восемь destination-пикселей по обеим осям. */
    gradient.matrix.x0 = 16.5; /* Выравниваем центр рампы с центром выбранного пикселя X=16. */
    for (stroke = 0; stroke < 2; ++stroke) /* Проверяем оба типа покрытия на том же пути. */
    { /* Начинаем серию трёх focal-позиций. */
        row = stroke ? 4 : 16; /* Выбираем верхнюю сторону обводки или внутренность заливки. */
        gradient.matrix.y0 = row + 0.5; /* Помещаем выбранные пиксели на горизонтальную ось градиента. */
        for (index = 0; index < 3; ++index) /* Изменяем только явный focal-параметр. */
        { /* Начинаем независимую отрисовку одного смещения. */
            memset(surface->pixels, 0, surface->stride * surface->height); /* Очищаем принадлежащий main буфер. */
            if (stroke) /* Для обводки выбираем положительную ширину четыре пикселя. */
                result = fvm_path_stroke_gradient_focal(path, surface, &gradient, 4, FVM_GRADIENT_SPREAD_PAD, FVM_GRADIENT_INTERPOLATION_RGB, focals[index]); /* Передаём внутренний focal без нового struct-поля. */
            else /* Заливка сохраняет прежнее правило nonzero. */
                result = fvm_path_draw_gradient_focal(path, surface, &gradient, 0, FVM_GRADIENT_SPREAD_PAD, FVM_GRADIENT_INTERPOLATION_RGB, focals[index]); /* Заимствуем путь, stops и буфер до возврата. */
            if (result != FVM_OK) /* Не читаем пиксели после ошибки подготовки или backend. */
            { /* Начинаем диагностику с сохранением владения main. */
                fprintf(stderr, "focal drawing failed: %d\n", (int)result); /* Печатаем исходный код ошибки. */
                return result; /* Передаём ошибку в общую очистку пути. */
            } /* Завершаем обработку ошибки. */
            printf("focal %s %s: %u %u %u\n", stroke ? "stroke" : "fill", names[index], /* Подписываем тип покрытия и focal. */
                   (unsigned int)surface->pixels[row * surface->stride + 12 * 4], /* Читаем красную компоненту слева от центра. */
                   (unsigned int)surface->pixels[row * surface->stride + 16 * 4], /* Читаем красную компоненту в центре. */
                   (unsigned int)surface->pixels[row * surface->stride + 20 * 4]); /* Читаем красную компоненту справа от центра. */
        } /* Завершаем три положения; последнее остаётся в буфере. */
    } /* Завершаем заливку и обводку без сохранения заимствованных указателей. */
    return FVM_OK; /* Сообщаем успех без создания VM, root или устройства. */
} /* Завершаем helper; локальные stops больше не используются. */
