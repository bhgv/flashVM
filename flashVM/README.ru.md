Версия разработки: **0.1505** ([VERSION.txt](VERSION.txt)). Все стадии
[роудмапа](doc/ROADMAP.md) остаются открытыми; текущие доказательства и
ограничения описаны в [приёмке](doc/RELEASE_ACCEPTANCE.md).
По действующему поручению основным временно является [GitHub](https://github.com/bhgv/flashVM):
изменения готовятся в локальной рабочей ветке и выдаются format-patch. Агент не
коммитит в master и не делает push. Пользователь применяет патч через git am --3way
и публикует master из Termux; точные команды выдаются с каждым патчем.
Author и committer: `bhgv <bhgv@bhgv.bhgv>`.
Флаг `--jit-emulator` записывает новые проверки инструкций/ABI в Unicorn;
[условия проверки](doc/PLATFORM_VALIDATION.ru.md).


v0.1505: [Масштабированные расстояния публичной round-обводки](doc/PUBLIC_ROUND_SCALE.ru.md).

v0.1504: [Контрольная точка перенесённых кривых](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1503: [Связать численную политику перенесённых controls](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1502: [Регрессия перенесённой кривой: matrix](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1501: [Регрессия перенесённой кривой: transform round](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1500: [Регрессия перенесённой кривой: transform tiny](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1499: [Регрессия перенесённой кривой: transform](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1498: [Регрессия перенесённой кривой: round tiny](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1497: [Регрессия перенесённой кривой: round negative](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1496: [Регрессия перенесённой кривой: round](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1495: [Регрессия перенесённой кривой: repeated](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1494: [Регрессия перенесённой кривой: linear](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1493: [Регрессия перенесённой кривой: hump](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1492: [Регрессия перенесённой кривой: quadratic](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1491: [Регрессия перенесённой кривой: tiny](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1490: [Регрессия перенесённой кривой: origins](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1489: [Регрессия перенесённой кривой: negative](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1488: [Регрессия перенесённой кривой: diagonal](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1487: [Регрессия перенесённой кривой: horizontal](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1486: [Регрессия перенесённой кривой: analytic](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1485: [Точные локальные координаты близких перенесённых controls](doc/TRANSLATED_CURVE_EXTREMA.ru.md).

v0.1484: [Контрольная точка малых cubic-экстремумов](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1483: [Исполняемый C89-пример малых cubic-экстремумов](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1482: [Экстремумы малых cubic: threads](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1481: [Экстремумы малых cubic: sampling](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1480: [Экстремумы малых cubic: contours](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1479: [Экстремумы малых cubic: transform](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1478: [Экстремумы малых cubic: append](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1477: [Экстремумы малых cubic: monotone](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1476: [Экстремумы малых cubic: repeated](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1475: [Экстремумы малых cubic: linear](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1474: [Экстремумы малых cubic: quadratic](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1473: [Экстремумы малых cubic: round](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1472: [Экстремумы малых cubic: transition](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1471: [Экстремумы малых cubic: scales](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1470: [Экстремумы малых cubic: diagonal](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1469: [Экстремумы малых cubic: vertical](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1468: [Экстремумы малых cubic: horizontal](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1467: [Экстремумы малых cubic: hump](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1466: [Экстремумы малых cubic: analytic](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1465: [Экстремумы малых cubic: масштабирование производной](doc/CUBIC_EXTREMA_SCALING.ru.md).

v0.1464: [Контрольная точка устойчивых Y-границ обводки](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1463: [Устойчивая Y-метрика обводки: threads](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1462: [Устойчивая Y-метрика обводки: transfer](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1461: [Устойчивая Y-метрика обводки: gc](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1460: [Устойчивая Y-метрика обводки: contours](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1459: [Устойчивая Y-метрика обводки: curves](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1458: [Устойчивая Y-метрика обводки: rect](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1457: [Устойчивая Y-метрика обводки: hitbox](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1456: [Устойчивая Y-метрика обводки: dimensions](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1455: [Устойчивая Y-метрика обводки: quality](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1454: [Устойчивая Y-метрика обводки: paints](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1453: [Устойчивая Y-метрика обводки: dominance](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1452: [Устойчивая Y-метрика обводки: reflection](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1451: [Устойчивая Y-метрика обводки: transition](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1450: [Устойчивая Y-метрика обводки: normal](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1449: [Устойчивая Y-метрика обводки: overflow](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1448: [Устойчивая Y-метрика обводки: subnormal](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1447: [Устойчивая Y-метрика обводки: underflow](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1446: [Устойчивая Y-метрика обводки: измерение](doc/SECONDARY_AXIS_BOUNDS.ru.md).

v0.1445: [Контрольная точка субнормальной геометрии штрихов](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1444: [Классификация точек малых bevel-соединений](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1443: [Попадание и отсечение для малых круглых торцов](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1442: [Устойчивые координаты малых bevel-соединений](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1441: [Устойчивое расстояние до коротких круглых торцов](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1440: [Проверки точности коротких штрихов: managed](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1439: [Проверки точности коротких штрихов: hits](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1438: [Проверки точности коротких штрихов: degenerate](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1437: [Проверки точности коротких штрихов: contours](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1436: [Проверки точности коротких штрихов: closed](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1435: [Проверки точности коротких штрихов: joins](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1434: [Проверки точности коротких штрихов: cubic](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1433: [Проверки точности коротких штрихов: quadratic](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1432: [Проверки точности коротких штрихов: reverse](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1431: [Проверки точности коротких штрихов: steep](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1430: [Проверки точности коротких штрихов: shallow](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1429: [Проверки точности коротких штрихов: diagonal](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1428: [Проверки точности коротких штрихов: нормализация](doc/SUBNORMAL_STROKE_NORMS.ru.md).

v0.1427: [Контрольная точка проверок коротких штрихов](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1426: [Нормализация коротких штрихов и регрессионные проверки](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1425: [Нормализация коротких штрихов и регрессионные проверки](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1424: [Нормализация коротких штрихов и регрессионные проверки](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1423: [Нормализация коротких штрихов и регрессионные проверки](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1422: [Нормализация коротких штрихов и регрессионные проверки](doc/SCALED_STROKE_TANGENTS.ru.md).

v0.1421: [Проверенная контрольная точка адаптивных линейных границ](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1420: [Компактная кривая через реальный C89 curveTo](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1419: [Независимые потоки и GC](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1418: [Binary/channel после teardown](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1417: [Качество и виды paint](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1416: [Bounding hitTest](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1415: [Размеры MovieClip](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1414: [Смешанная заливка](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1413: [Hairline](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1412: [Managed getBounds/getRect](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1411: [Изолированные Move](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1410: [Избыточная разрядность](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1409: [Underflow нормы тангенса](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1408: [Потеря остатка вычитания](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1407: [Малые нормализуемые тангенсы](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1406: [Двоичные масштабы](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1405: [Большие и дробные переносы](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1404: [Обратное направление](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1403: [Компактные cubic](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1402: [Компактные quadratic](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1401: [Адаптивные точные векторы линейных Bezier](doc/ADAPTIVE_CURVE_BOUNDS.ru.md).

v0.1400: [Проверенная контрольная точка линейных Bezier](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1399: [Диагональная кривая через реальный C89 curveTo](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1398: [Независимые потоки и GC](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1397: [Binary/channel после teardown](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1396: [Качество и виды paint](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1395: [Bounding hitTest](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1394: [Размеры MovieClip](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1393: [Смешанная заливка](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1392: [Hairline](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1391: [Managed getBounds/getRect](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1390: [Соединения и замыкание](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1389: [Изолированные Move](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1388: [Отказы без изменения результатов](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1387: [Пределы координат и ширины](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1386: [Поворот и масштаб](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1385: [Дробная двоичная сетка](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1384: [Стационарная середина cubic](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1383: [Обратное направление](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1382: [Неупорядоченные collinear cubic](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1381: [Коллинеарные quadratic](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1380: [Точное сокращение линейных Bezier на двоичной сетке](doc/LINEAR_CURVE_BOUNDS.ru.md).

v0.1379: [Проверенная контрольная точка монотонных осевых кривых](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1378: [Продемонстрированы точные осевые bounds через curveTo](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1377: [Независимые OS-потоки](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1376: [Передача графов binary/channel](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1375: [Виды paint и качество Stage](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1374: [Bounding hitTest](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1373: [Размеры и равномерный масштаб](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1372: [Смешанная заливка и штрих](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1371: [Managed hairline](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1370: [Managed bounds и getRect](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1369: [Дробная и предельная ширина](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1368: [Сохранность результатов при отказе](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1367: [Соединения и замыкание](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1366: [Изолированные и постоянные контуры](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1365: [Постоянные команды](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1364: [Нулевые производные торцов](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1363: [Обратное направление](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1362: [Вертикальная ось](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1361: [Cubic: все торцы и соединения](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1360: [Quadratic: все торцы и соединения](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1359: [Добавлены точные границы монотонных осевых кривых](doc/AXIS_CURVE_BOUNDS.ru.md).

v0.1358: [Проверенная контрольная точка аналитических round-кривых](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1357: [Добавлен C89-разбор round-кривой с GC-корнями](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1356: [Независимые рабочие потоки](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1355: [Передача binary и channel](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1354: [Снимки стиля clear и GC](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1353: [Качество Stage и виды paint](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1352: [Рамка hitTest без shape](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1351: [Размеры hairline-кривой](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1350: [Смешанные заливка и штрих](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1349: [Отказ managed cubic и восстановление](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1348: [Постоянные managed-кривые и getRect](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1347: [Одиночные moveTo в managed-кривой](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1346: [Сохранение обоих результатов при ошибке](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1345: [Пределы координат и ширины](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1344: [Поворот и равномерный масштаб](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1343: [Объединение активных контуров](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1342: [Постоянные сегменты](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1341: [Непостоянная возвратная кривая](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1340: [Кубические экстремумы](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1339: [Квадратичные экстремумы](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1338: [Добавлены аналитические round-границы Bezier](doc/ROUND_CURVE_BOUNDS.ru.md).

v0.1337: [Проверенная контрольная точка round и hairline bounds](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1336: [Документированы пределы ширины и владение геометрией](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1335: [Регрессия round bounds: gc](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1334: [Регрессия round bounds: quality](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1333: [Регрессия round bounds: hitbox](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1332: [Регрессия round bounds: dimensions](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1331: [Регрессия round bounds: curves](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1330: [Регрессия round bounds: closed](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1329: [Регрессия round bounds: degenerate](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1328: [Регрессия round bounds: isolated_moves](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1327: [Регрессия round bounds: diagonal](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1326: [Регрессия round bounds: hairline](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1325: [Объединены прямые round bounds и оболочки hairline](doc/ROUND_STROKE_BOUNDS.ru.md).

v0.1324: [Проверенные script-параметры линии и оставшаяся приёмка](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1323: [Исполняемый C89-пример стилей линии](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1322: [Действия кадров и перемотка стилей](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1321: [Эквивалентность JIT с нативными входами](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1320: [Реальные вызовы AVM1 lineStyle](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1319: [Изолированные потоки script-линий](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1318: [Перенесённые графы script-линий](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1317: [Независимое дублированное состояние линии](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1316: [Атомарная смена стиля при отказах памяти](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1315: [Порядок преобразований и восстановление callbacks](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1314: [Независимые стили заливки и линии](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1313: [Границы и попадания script-линий](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1312: [Выбранные линии и качество Stage](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1311: [Линии кривых и углов](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1310: [Выбранные градиентные линии](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1309: [Независимые смены стилей путей](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1308: [Восстановление после неподдержанных опций](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1307: [Версионные параметры линий](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1306: [Ограничение ширин и значения по умолчанию](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1305: [Сохранённые стили script-линий](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1304: [Атомарные cap/join стили script Flash8](doc/SCRIPT_STROKE_STYLES.ru.md).

v0.1303: [Проверенные границы выбранных штрихов и оставшаяся приёмка](doc/STROKE_BOUNDS.ru.md).

v0.1302: [Восстановление границ после отказов памяти](doc/STROKE_BOUNDS.ru.md).

v0.1301: [Границы изолированных потоков](doc/STROKE_BOUNDS.ru.md).

v0.1300: [Границы таймлайна и перемотка](doc/STROKE_BOUNDS.ru.md).

v0.1299: [Границы перенесённых графов после удаления отправителя](doc/STROKE_BOUNDS.ru.md).

v0.1298: [Независимость от качества растра](doc/STROKE_BOUNDS.ru.md).

v0.1297: [Размеры и геометрические hit boxes](doc/STROKE_BOUNDS.ru.md).

v0.1296: [Объединение заливки, штриха и пустых контуров](doc/STROKE_BOUNDS.ru.md).

v0.1295: [Преобразованные и схлопнутые границы](doc/STROKE_BOUNDS.ru.md).

v0.1294: [Восстановленные метаданные границ](doc/STROKE_BOUNDS.ru.md).

v0.1293: [Консервативные границы кривых](doc/STROKE_BOUNDS.ru.md).

v0.1292: [Границы hairline и крайних ширин](doc/STROKE_BOUNDS.ru.md).

v0.1291: [Восстановленные границы SWF и getRect](doc/STROKE_BOUNDS.ru.md).

v0.1290: [Атомарное отклонение запроса границ](doc/STROKE_BOUNDS.ru.md).

v0.1289: [Границы контуров и пустых штрихов](doc/STROKE_BOUNDS.ru.md).

v0.1288: [Внешние экстремумы соединений](doc/STROKE_BOUNDS.ru.md).

v0.1287: [Независимые диагональные оболочки](doc/STROKE_BOUNDS.ru.md).

v0.1286: [Аналитические границы торцов](doc/STROKE_BOUNDS.ru.md).

v0.1285: [Границы прямых штрихов по торцам и соединениям](doc/STROKE_BOUNDS.ru.md).

v0.1284: [Проверенный shape hitTest стилей и оставшиеся ограничения](doc/STROKE_HIT_TEST.ru.md).

v0.1283: [Восстановление после отказов памяти](doc/STROKE_HIT_TEST.ru.md).

v0.1282: [Запросы изолированных потоков](doc/STROKE_HIT_TEST.ru.md).

v0.1281: [Запросы таймлайна и перемотки](doc/STROKE_HIT_TEST.ru.md).

v0.1280: [Запросы перенесённых графов линий](doc/STROKE_HIT_TEST.ru.md).

v0.1279: [Независимость от качества Stage](doc/STROKE_HIT_TEST.ru.md).

v0.1278: [Запросы равномерных преобразований](doc/STROKE_HIT_TEST.ru.md).

v0.1277: [Валидация восстановленных selectors](doc/STROKE_HIT_TEST.ru.md).

v0.1276: [Запросы крайних ширин SWF](doc/STROKE_HIT_TEST.ru.md).

v0.1275: [Запросы восстановленных SWF-красок](doc/STROKE_HIT_TEST.ru.md).

v0.1274: [Совместимость прежнего round](doc/STROKE_HIT_TEST.ru.md).

v0.1273: [Независимый плотный геометрический эталон](doc/STROKE_HIT_TEST.ru.md).

v0.1272: [Валидация аргументов и эквивалентность заливки](doc/STROKE_HIT_TEST.ru.md).

v0.1271: [Запросы обводки кривых](doc/STROKE_HIT_TEST.ru.md).

v0.1270: [Топология и вырожденные контуры](doc/STROKE_HIT_TEST.ru.md).

v0.1269: [Внешние сектора и короткие рёбра](doc/STROKE_HIT_TEST.ru.md).

v0.1268: [Аналитическое покрытие торцов](doc/STROKE_HIT_TEST.ru.md).

v0.1267: [Shape hitTest с учётом торцов и соединений](doc/STROKE_HIT_TEST.ru.md).

v0.1266: [Проверенный этап симметричных концов и bevel с чистыми нативными sanitizer и SDK-free проверками](doc/STROKE_CAPS.ru.md).

v0.1265: [Двадцать четыре банка StateNewStyles сохраняют селекторы и собственные записи paint](doc/STROKE_CAPS.ru.md).

v0.1264: [Шесть смешанных сцен сохраняют порядок композиции и отдельную геометрию обводки](doc/STROKE_CAPS.ru.md).

v0.1263: [Четыре потока сохраняют геометрию концов и соединений в 96 независимых циклах](doc/STROKE_CAPS.ru.md).

v0.1262: [Замена персонажа и шестнадцать перемоток восстанавливают стили bitmap и градиентных обводок](doc/STROKE_CAPS.ru.md).

v0.1261: [Двадцать четыре серии отказов сохраняют неопубликованные графы и выходные пиксели](doc/STROKE_CAPS.ru.md).

v0.1260: [Двадцать четыре сцены paint и стиля переживают перенос и удаление отправителя](doc/STROKE_CAPS.ru.md).

v0.1259: [Повреждённые скаляры отклоняются до записи а отсутствующие поля восстанавливают round](doc/STROKE_CAPS.ru.md).

v0.1258: [Неверные селекторы усечение разные концы и miter сохраняют прежний результат](doc/STROKE_CAPS.ru.md).

v0.1257: [96 сочетаний качества paint и стиля сохраняют политику покрытия](doc/STROKE_CAPS.ru.md).

v0.1256: [96 топологий сохраняют выбранные концы соединения и команды замыкания](doc/STROKE_CAPS.ru.md).

v0.1255: [120 сочетаний paint стиля и ширины сохраняют выбранную геометрию обводки](doc/STROKE_CAPS.ru.md).

v0.1254: [Независимые прямоугольники круглые углы и срезы проверяют геометрию концов и соединений](doc/STROKE_CAPS.ru.md).

v0.1253: [Симметричные butt square round концы и bevel проходят через все типы обводок](doc/STROKE_CAPS.ru.md).

v0.1252: [Проверенный этап bitmap-обводок LINESTYLE2 с нативными sanitizer и SDK-free проверками](doc/BITMAP_STROKES.ru.md).

v0.1251: [Новые банки bitmap-обводок сохраняют NoClose владение ID изображений и смещения](doc/BITMAP_STROKES.ru.md).

v0.1250: [Смешанные сцены сохраняют разные изображения заливки и обводки и порядок пикселей](doc/BITMAP_STROKES.ru.md).

v0.1249: [Четыре потока выполняют 96 независимых циклов загрузки bitmap-обводок рендеринга и GC](doc/BITMAP_STROKES.ru.md).

v0.1248: [Замена персонажа и шестнадцать перемоток сохраняют начало и режим bitmap-семплинга](doc/BITMAP_STROKES.ru.md).

v0.1247: [Четыре bitmap-режима проходят отказы памяти декодера загрузчика и подготовки paint](doc/BITMAP_STROKES.ru.md).

v0.1246: [Проверены ссылки на изображения словаря живые изменения dispose и неверные метаданные](doc/BITMAP_STROKES.ru.md).

v0.1245: [Неверные записи ссылки версии ширины и aliasing проверяются до записи пикселей](doc/BITMAP_STROKES.ru.md).

v0.1244: [Копии сцены через сериализацию и канал рисуют bitmap-обводки после удаления источника](doc/BITMAP_STROKES.ru.md).

v0.1243: [Шестнадцать сочетаний качества Stage и bitmap-режима сохраняют независимую фильтрацию](doc/BITMAP_STROKES.ru.md).

v0.1242: [Двадцать сдвинутых обводок сохраняют repeat продолжение края и RGBA](doc/BITMAP_STROKES.ru.md).

v0.1241: [Шестнадцать открытых замкнутых и NoClose bitmap-контуров сохраняют команды и пиксели](doc/BITMAP_STROKES.ru.md).

v0.1240: [Двадцать сочетаний режима и ширины включают hairline и максимальную ширину](doc/BITMAP_STROKES.ru.md).

v0.1239: [Bitmap-обводки LINESTYLE2 сохраняют независимые изображения и подготовленное круглое покрытие](doc/BITMAP_STROKES.ru.md).

v0.1238: [Проверенный этап круглых LINESTYLE2-обводок с нативными sanitizer и чистой безграфической сборкой](doc/SHAPE4_STROKES.ru.md).

v0.1237: [StateNewStyles восстанавливает собственные банки фокусных NoClose-обводок и смещения](doc/SHAPE4_STROKES.ru.md).

v0.1236: [Смешанные сцены сохраняют независимые spread интерполяцию и фокус заливки и обводки](doc/SHAPE4_STROKES.ru.md).

v0.1235: [Четыре OS-потока выполняют 96 независимых циклов загрузки обводок и GC](doc/SHAPE4_STROKES.ru.md).

v0.1234: [Замена персонажа и перемотки восстанавливают независимые режимы обводки](doc/SHAPE4_STROKES.ru.md).

v0.1233: [Шесть NoClose-конфигураций с 15 точками проходят отказы памяти декодера и сцены](doc/SHAPE4_STROKES.ru.md).

v0.1232: [Копии через сериализацию и канал сохраняют NoClose и фокусную обводку после GC](doc/SHAPE4_STROKES.ru.md).

v0.1231: [Усечение вложенных стилей резервные режимы границы фокуса и bitmap-обводки отклоняются явно](doc/SHAPE4_STROKES.ru.md).

v0.1230: [Все 511 внутренних FIXED8-фокусов обводки сохраняют точное значение](doc/SHAPE4_STROKES.ru.md).

v0.1229: [180 сочетаний числа точек типа и цветового режима обводки отрисовываются](doc/SHAPE4_STROKES.ru.md).

v0.1228: [Все 65536 заголовков различают повреждённые неподдержанные и допустимые записи](doc/SHAPE4_STROKES.ru.md).

v0.1227: [Открытые замкнутые и NoClose пути сохраняют команды и поведение концов](doc/SHAPE4_STROKES.ru.md).

v0.1226: [Hairline субпиксельная и максимальная UI16-ширина совпадают с публичным рендерингом](doc/SHAPE4_STROKES.ru.md).

v0.1225: [Круглые сплошные и градиентные LINESTYLE2-обводки с геометрией NoClose](doc/SHAPE4_STROKES.ru.md).

v0.1224: [Проверенный этап заливок Shape4 с нативными sanitizer и безграфическими проверками](doc/SHAPE4_FILLS.ru.md).

v0.1223: [Двуязычные контракты Shape4 ссылки на исходники и уточнение прежних ограничений](doc/SHAPE4_FILLS.ru.md).

v0.1222: [Четыре OS-потока выполняют 96 независимых циклов загрузки рендеринга и GC](doc/SHAPE4_FILLS.ru.md).

v0.1221: [Замена персонажа и шестнадцать перемоток восстанавливают фокусные цвета](doc/SHAPE4_FILLS.ru.md).

v0.1220: [Четыре режима bitmap-заливок Shape4 отрисовываются и проверяют ресурсы](doc/SHAPE4_FILLS.ru.md).

v0.1219: [Проверены порядок и пределы координат EdgeBounds и подсказки обводки](doc/SHAPE4_FILLS.ru.md).

v0.1218: [RGBA-банки StateNewStyles сохраняют смещения и отклоняют LINESTYLE2](doc/SHAPE4_FILLS.ru.md).

v0.1217: [Проверки отказов памяти сохраняют старую геометрию и исключают частичную публикацию сцены](doc/SHAPE4_FILLS.ru.md).

v0.1216: [Загрузчик отклоняет старые версии повторные ID и вложенные определения](doc/SHAPE4_FILLS.ru.md).

v0.1215: [Копии сцены через сериализацию и канал отрисовываются после удаления исходного контекста и GC](doc/SHAPE4_FILLS.ru.md).

v0.1214: [Все 511 внутренних FIXED8-фокусов точно декодируются и отрисовываются](doc/SHAPE4_FILLS.ru.md).

v0.1213: [Все 1–15 точек градиента проверены с режимами spread и интерполяции](doc/SHAPE4_FILLS.ru.md).

v0.1212: [Ошибки усечения резервных битов и границ фокуса сохраняют прежний результат](doc/SHAPE4_FILLS.ru.md).

v0.1211: [Ограниченные заливки DefineShape4 с режимами SWF8 и знаковым внутренним фокусом](doc/SHAPE4_FILLS.ru.md).

v0.1210: [Записаны native sanitizer SDK-free и detached focal проверки с открытыми endpoints](doc/FOCAL_GRADIENTS.ru.md).

v0.1209: [Проверены raw focal metadata и mixed origins с двуязычными interior-only контрактами](doc/FOCAL_GRADIENTS.ru.md).

v0.1208: [Добавлены строгий focal pixel checker CTest и пояснённый пример AS2 focal](doc/FOCAL_GRADIENTS.ru.md).

v0.1207: [Добавлен пояснённый focal tutorial с измерением пикселей и явным временем заимствования](doc/FOCAL_GRADIENTS.ru.md).

v0.1206: [Проверено сохранение centered state и пикселей при ошибках выделения signed focal](doc/FOCAL_GRADIENTS.ru.md).

v0.1205: [Проверены 192 сочетания signed focal заливки обводки цвета и качества](doc/FOCAL_GRADIENTS.ru.md).

v0.1204: [Проверены четыре OS worker и 128 независимых циклов signed focal paint и GC](doc/FOCAL_GRADIENTS.ru.md).

v0.1203: [Проверены 144 focal конфигурации исполнения с native counters и атомарным отклонением](doc/FOCAL_GRADIENTS.ru.md).

v0.1202: [Проверены владение focal-графом и независимый centered fallback после уничтожения отправителя](doc/FOCAL_GRADIENTS.ru.md).

v0.1201: [Проверены ошибки valueOf эффекты и успешное копирование radial focal coercion](doc/FOCAL_GRADIENTS.ru.md).

v0.1200: [Проверены аналитические лучевые контуры и устойчивая выборка внутренних focal](doc/FOCAL_GRADIENTS.ru.md).

v0.1199: [Подключены focal metadata к выборке заливок обводок цвета и качества](doc/FOCAL_GRADIENTS.ru.md).

v0.1198: [Добавлены optional focal metadata и допуск радиальных аргументов Flash8](doc/FOCAL_GRADIENTS.ru.md).

v0.1197: [Добавлен API внутренних focal-позиций и проверки прежних пикселей при focal zero](doc/FOCAL_GRADIENTS.ru.md).

v0.1196: [Записаны native sanitizer SDK-free и detached проверки без закрытия этапов roadmap](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1195: [Проверены raw metadata и mixed policy graphs с двуязычными контрактами interpolation](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1194: [Добавлены точный midpoint checker CTest и пояснённый SWF8 пример цвета](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1193: [Добавлен пояснённый C89 пример цветовых политик с заимствованием и очисткой](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1192: [Проверено сохранение старых стилей и пикселей при ошибках подготовки заливок и обводок](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1191: [Проверены 96 сочетаний заливки обводки преобразования и качества](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1190: [Проверены четыре OS worker и 128 независимых циклов linearRGB paint и GC](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1189: [Проверены 72 сочетания исполнения linearRGB host и SWF8 с native counters](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1188: [Проверены копирование аргументов и независимость interpolation после уничтожения отправителя](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1187: [Проверены VM и native callback ошибки без частичной публикации interpolation](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1186: [Проверены transfer breakpoints duplicate stops source-over и цветовые преобразования](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1185: [Подключены проверенные interpolation metadata к выборке заливок и обводок](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1184: [Добавлены версионированные counted interpolation names и optional metadata](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1183: [Добавлены явные interpolation selectors с декодированием stops на одну отрисовку](doc/LINEAR_RGB_GRADIENTS.ru.md).

v0.1182: [Проверены mixed ramp ownership и native sanitizer SDK-free detached проверки](doc/GRADIENT_STROKES.ru.md).

v0.1181: [Описаны C89 и AS2 контракты обводок на английском и русском](doc/GRADIENT_STROKES.ru.md).

v0.1180: [Добавлены checker фазовых выборок CTest и пояснённый пример AS2 обводки](doc/GRADIENT_STROKES.ru.md).

v0.1179: [Добавлен запускаемый пример обводок и описаны фазовые выборки](doc/GRADIENT_STROKES.ru.md).

v0.1178: [Проверены C89 layout ограничения и неверные metadata восстановленных обводок](doc/GRADIENT_STROKES.ru.md).

v0.1177: [Проверены преобразования обводок и локальная политика antialias сцены](doc/GRADIENT_STROKES.ru.md).

v0.1176: [Проверены четыре OS worker и 128 циклов заливки обводки и GC](doc/GRADIENT_STROKES.ru.md).

v0.1175: [Проверено сохранение active path и пикселей при ошибках выделения обводок](doc/GRADIENT_STROKES.ru.md).

v0.1174: [Проверены ошибки coercion обводок и очистка native GC barrier](doc/GRADIENT_STROKES.ru.md).

v0.1173: [Проверены 72 сочетания host и SWF8 с контролем native entry counts](doc/GRADIENT_STROKES.ru.md).

v0.1172: [Проверены независимое владение графом обводки и уничтожение отправителя](doc/GRADIENT_STROKES.ru.md).

v0.1171: [Подключены рисование обводок сброс стиля и владение графом дубликата](doc/GRADIENT_STROKES.ru.md).

v0.1170: [Добавлен версионированный стиль обводки с копированием и стабильным callback ID](doc/GRADIENT_STROKES.ru.md).

v0.1169: [Добавлен API градиентных обводок с общей выборкой и подготовленной маской](doc/GRADIENT_STROKES.ru.md).

v0.1168: [Описаны spread contracts и проверены native sanitizer и SDK-free проверки](doc/GRADIENT_SPREAD.ru.md).

v0.1167: [Проверены C89 selectors и staged gradient/raster allocation failures](doc/GRADIENT_SPREAD.ru.md).

v0.1166: [Проверены spread graphs после duplication, transfer, GC и sender teardown](doc/GRADIENT_SPREAD.ru.md).

v0.1165: [Проверены host/Flash8 spread state и прежнее отклонение старых версий](doc/GRADIENT_SPREAD.ru.md).

v0.1164: [Проверены analytic C89 gradient pixels, large phases и прежний pad output](doc/GRADIENT_SPREAD.ru.md).

v0.1163: [Managed gradient rendering проверяет optional modes и сохраняет legacy pad records](doc/GRADIENT_SPREAD.ru.md).

v0.1162: [beginGradientFill готовит поддержанные spread modes в owned descriptors](doc/GRADIENT_SPREAD.ru.md).

v0.1161: [Реализованы repeat и reflect phases с сохранением прежнего pad sampling](doc/GRADIENT_SPREAD.ru.md).

v0.1160: [Добавлен spread-aware C89 entry с сохранением прежнего gradient layout](doc/GRADIENT_SPREAD.ru.md).

v0.1159: [Зафиксированы visible-line проверки и allocation coverage с регистрациями и без них](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1158: [Проверены bottom-only gates и ownership для 18 native edit-комбинаций](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1157: [Документированы bottom-visible-line транзакции и bounded descriptor discovery](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1156: [C89 observer trace дополнен bottom-only text changes и visible lines](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1155: [Проверены перенесённые bottom-only observers и 128 worker эпох](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1154: [Проверены 27 bottom-only AVM1 edit/execution комбинаций](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1153: [Проверены bottom-only edit-транзакции на границах ошибок](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1152: [Проверены nested state и удалённые recipients в bottom-only events](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1151: [Проверены charged large-table lookup и work failures до и после commit](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1150: [Проверен descriptor-only lookup для 24 version/name/kind комбинаций](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1149: [Проверены bottomScroll-only события от text и replacement](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1148: [Изменения видимой нижней строки вызывают field-first scroll observers](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1147: [Geometry и content observer admission используют общий charged lookup](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1146: [Подготовлен descriptor-only observer lookup с bounded folded traversal](doc/TEXT_VISIBLE_EVENTS.ru.md).

v0.1145: [Зафиксированы edit observer проверки и оставшиеся ограничения плеера](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1144: [Проверены версии SWF, identical loaded text, отсутствие onChanged и autoSize anchors](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1143: [Документированы content/style транзакции и оставшийся input/display-list объём](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1142: [C89 observer example дополнен text, replacement и current-format ranges](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1141: [Проверены binary/channel text events и 128 worker edit/GC эпох](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1140: [Проверены 27 реальных AVM1 edit-комбинаций SWF6–8 и execution modes](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1139: [Проверены три edit-транзакции на allocation и work boundaries](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1138: [Проверено сохранение nested text для трёх внешних операций](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1137: [Проверены диапазоны format, kerning, leading и silent future/decoration](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1136: [Проверены replaceText events, counted NUL, alias и no-op](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1135: [Проверены text и диапазоны внутри field-first callbacks](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1134: [Current TextFormat публикуется до observers; future style остаётся без события](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1133: [replaceText готовит recipients до владения и публикации текста](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1132: [Присваивание text уведомляет об effective изменениях диапазонов](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1131: [Подготовлена геометрия proposed content/style без публикации состояния](doc/TEXT_EDIT_EVENTS.ru.md).

v0.1130: [Зафиксированы проверки геометрии и оставшиеся ограничения совместимости](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1129: [Документированы триггеры геометрии, eager admission и оставшиеся text events](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1128: [Допуск геометрии учитывает folding handlers и отложенный dynamic resolve](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1127: [C89 tutorial дополнен изменением диапазонов при записи размеров](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1126: [Проверены receiver-owned события, версии SWF и 128 worker эпох](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1125: [Проверены geometry setters SWF6–8 в interpreter и native JIT](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1124: [Проверены rollback до публикации и частичная доставка после commit](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1123: [Проверены вложенная геометрия и удалённые snapshot recipients](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1122: [Проверены wrapping, autoSize и отклонение multiline без callback](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1121: [Проверены диапазоны размеров, clamp, source identity и неизменные записи](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1120: [Режимы раскладки синхронно уведомляют об изменении диапазонов](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1119: [Изменения width и height публикуют диапазоны до observers](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1118: [Подготовлены rooted-транзакции геометрии и observers](doc/TEXT_GEOMETRY_EVENTS.ru.md).

v0.1117: [Зафиксированы native, sanitizer, SDK-free и detached проверки observers](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1116: [Проверены ошибки callback, старые alias, версии SWF и циклы listeners](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1115: [Документированы ошибки observers, передача графа и оставшийся ввод](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1114: [Разделены precommit и postcommit work errors; регистрация сохраняется при исчерпании budget.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1113: [Проверены отдельные VM/listeners/callback за 128 эпох native OS-worker.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1112: [C89-пример проверяет порядок field-first и срок жизни host userdata до уничтожения VM.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1111: [Проверены эффекты событий, source argument и настоящие native entries для SWF6–8.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1110: [Перебраны allocation boundaries до/после commit и проверено снятие roots.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1109: [Listener graph передаётся binary/channel; upgrade ограничен retired rootless methods.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1108: [Удалённые recipients живут до конца текущей доставки; вложенная позиция сохраняется.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1107: [Проверены порядок, identity, удаление, unchanged positions и no-script наблюдения.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1106: [После публикации уведомляются field и snapshot listeners; вложенные изменения сохраняются.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1105: [Field и listener graph удерживаются с общим execution budget для вложенной доставки.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1104: [Добавлены регистрация/удаление identity и стабильные callback graph IDs.](doc/TEXT_SCROLL_EVENTS.ru.md).

v0.1103: [Зафиксированы native, sanitizer, C89, transfer и detached проверки; все стадии открыты.](doc/TEXT_SCROLL.ru.md).

v0.1102: [Capture копирует эффективные позиции без изменения source; проверен строгий C89 ABI.](doc/TEXT_SCROLL.ru.md).

v0.1101: [При исключении conversion сохраняются viewport ownership и native GC barriers.](doc/TEXT_SCROLL.ru.md).

v0.1100: [Проверены отдельные VM, шрифты и контуры за 128 эпох настоящих OS-worker.](doc/TEXT_SCROLL.ru.md).

v0.1099: [Добавлен проверяемый C89-режим и oracle с явной ответственностью хоста за clipping.](doc/TEXT_SCROLL.ru.md).

v0.1098: [Проверены переносы, hard lines, underline, counted text и границы layout admission.](doc/TEXT_SCROLL.ru.md).

v0.1097: [Проверены interpreter/JIT, свежая геометрия после conversion и native GC barriers.](doc/TEXT_SCROLL.ru.md).

v0.1096: [Проверены все точки выделения query/export и сохранение активного viewport.](doc/TEXT_SCROLL.ru.md).

v0.1095: [Проверены независимые шрифты, контуры и viewport после уничтожения sender и GC.](doc/TEXT_SCROLL.ru.md).

v0.1094: [Проверены геометрия строк, нулевой viewport, метрики и сохранение выхода при ошибке.](doc/TEXT_SCROLL.ru.md).

v0.1093: [Смещения применены к glyph/underline; метрики не меняются, публикация после успешного export.](doc/TEXT_SCROLL.ru.md).

v0.1092: [Добавлено optional wire-состояние с прежними defaults и проверкой границ receiver.](doc/TEXT_SCROLL.ru.md).

v0.1091: [Добавлены свойства прокрутки, числовой clamp и upgrade старых rootless accessors.](doc/TEXT_SCROLL.ru.md).

v0.1090: [Добавлен расчёт диапазонов строк и пикселей по одностильной раскладке.](doc/TEXT_SCROLL.ru.md).

v0.1089: [Сохранён выбор кадров PCM и зафиксированы native, sanitizer, SDK-free и независимые проверки.](doc/MP3_FRAME_SEEK.ru.md).

v0.1088: [Работающий worker не отправляет PCM до авторского кадра и продолжает после frame entry и GC.](doc/MP3_FRAME_SEEK.ru.md).

v0.1087: [Опубликованы согласованные RU/EN контракты и ссылки из codec/output/roadmap.](doc/MP3_FRAME_SEEK.ru.md).

v0.1086: [Добавлено независимое сравнение полных хвостов FWS/CWS, задержки, EOF и сохранности вывода.](doc/MP3_FRAME_SEEK.ru.md).

v0.1085: [Проверены точный пик памяти, отказы выделений и независимые OS-потоки после VM.](doc/MP3_FRAME_SEEK.ru.md).

v0.1084: [Номера кадров и знаковые позиции проверяются до открытия выходного файла.](doc/MP3_FRAME_SEEK.ru.md).

v0.1083: [Проверены знаковые старты, пустые кадры, задержка и сброс очереди в интерпретаторе и JIT.](doc/MP3_FRAME_SEEK.ru.md).

v0.1082: [Desktop использует знаковую позицию и удерживает обратный переход до авторского кадра.](doc/MP3_FRAME_SEEK.ru.md).

v0.1081: [При ошибке сохранены PCM и оба конвертера; сброс выполняется после успешного clear.](doc/MP3_FRAME_SEEK.ru.md).

v0.1080: [Проверены шесть векторов, знаковые границы, точный конец и отклонение неверных запросов.](doc/MP3_FRAME_SEEK.ru.md).

v0.1079: [Знаковые позиции проверяются до callback и публикуются после успешной очистки.](doc/MP3_FRAME_SEEK.ru.md).

v0.1078: [Добавлен неизменяемый поиск позиции с проверкой знаковых границ и точного EOF.](doc/MP3_FRAME_SEEK.ru.md).

v0.1077: [Сохранены исходные префиксы и знаковые смещения без изменения непрерывного PCM.](doc/MP3_FRAME_SEEK.ru.md).

v0.1076: [Зафиксированы native, sanitizer, SDK-free, FFmpeg и desktop PCM проверки; неоднозначные длины, A/V и дочерние потоки остаются открытыми.](doc/INFERRED_ADPCM.ru.md).

v0.1075: [Добавлены post-VM export без counts и FFmpeg oracle для всех ширин/каналов, CWS/FWS, partial restart и пустых блоков.](doc/INFERRED_ADPCM.ru.md).

v0.1074: [Описаны canonical compatibility, unique partial endpoints, counted API, peak budget и границы native/physical приёмки RU/EN.](doc/INFERRED_ADPCM.ru.md).

v0.1073: [Post-VM PCM сохраняется при rejected write, gate, failed clear и reset; финальный short retry даёт ровно 19 stereo frames.](doc/INFERRED_ADPCM.ru.md).

v0.1072: [Четыре OS-worker читают общий post-VM snapshot с разными курсорами, проверяя интерполяцию и restart в 128 эпохах.](doc/INFERRED_ADPCM.ru.md).

v0.1071: [C89-пример добавляет inferred-режим без таблицы; все три режима проверяются по восьми ручным samples.](doc/INFERRED_ADPCM.ru.md).

v0.1070: [Worker проверяет single-saturation stereo PCM и BASE19/TAIL0 для обоих finite-режимов, CWS/FWS и двух VM modes.](doc/INFERRED_ADPCM.ru.md).

v0.1069: [Проверены FWS/CWS, interpreter/JIT, точный PCM, restart, пустые блоки, PCM successor и отказ до устройства.](doc/INFERRED_ADPCM.ru.md).

v0.1068: [Desktop-сегменты используют inferred bounded snapshot с общим peak-бюджетом и ошибкой до открытия устройства.](doc/INFERRED_ADPCM.ru.md).

v0.1067: [Проверены неверные окончания, короткий predictor, неоднозначный mono и единственное nonzero-окончание с точным PCM.](doc/INFERRED_ADPCM.ru.md).

v0.1066: [Проверены четыре native-аллокации, точный и недостаточный бюджет, уничтожение VM и чтение PCM без аллокаций.](doc/INFERRED_ADPCM.ru.md).

v0.1065: [Для 104 геометрий независимо перебираются длины и проверяется каждый PCM sample, неоднозначность и legacy whole packets.](doc/INFERRED_ADPCM.ru.md).

v0.1064: [Добавлена bounded-подготовка с прежними whole packets и единственной partial-длиной без header average.](doc/INFERRED_ADPCM.ru.md).

v0.1063: [Зафиксированы native, sanitizer, SDK-free и C89 проверки единого снимка; source-time и A/V приёмка остаются открытыми.](doc/MIXED_SNAPSHOT.ru.md).

v0.1062: [Режим наблюдения публикуется до worker; backend UNSUPPORTED сохраняется после одного callback с проверкой out и числа опросов.](doc/MIXED_SNAPSHOT.ru.md).

v0.1061: [Один PCM-эталон проверяет legacy и origin-enabled refill, точный retry, финальный drain и сохранение out при UNSUPPORTED.](doc/MIXED_SNAPSHOT.ru.md).

v0.1060: [Внутренний stream-запрос совместим с адаптерами без origins; явный paired API сохраняет UNSUPPORTED.](doc/MIXED_SNAPSHOT.ru.md).

v0.1059: [Описаны один callback, публикация stream cache, неоткатываемый прогресс очереди и desktop-владение.](doc/MIXED_SNAPSHOT.ru.md).

v0.1058: [Seeded-проверки охватывают границу точности clock, wrap, регрессию, target mismatch и сброс.](doc/MIXED_SNAPSHOT.ru.md).

v0.1057: [C89-пример выводит единые снимки до и после drain; независимый ten-mode PCM checker проверяет оба отчёта.](doc/MIXED_SNAPSHOT.ru.md).

v0.1056: [Mixed refill, resume, завершение и stream-диагностика используют единое наблюдение.](doc/MIXED_SNAPSHOT.ru.md).

v0.1055: [Worker SDL dummy проверяет суммы clock при authored Stop, Play, pause и GC в интерпретаторе и JIT.](doc/MIXED_SNAPSHOT.ru.md).

v0.1054: [Добавлен desktop-запрос под mutex для смешанной статистики; сохранена семантика EOF последовательности.](doc/MIXED_SNAPSHOT.ru.md).

v0.1053: [Четыре OS-потока проверяют PCM и один опрос на снимок в 256 эпохах; запрет аллокаций охватывает снимки при retry.](doc/MIXED_SNAPSHOT.ru.md).

v0.1052: [Проверены чужие владельцы, отключённый режим, ошибки и исключения callbacks, pending BASE, неудачный clear и сброс эпохи.](doc/MIXED_SNAPSHOT.ru.md).

v0.1051: [Добавлен единый снимок stream и BASE/TAIL без рендеринга и продвижения источника.](doc/MIXED_SNAPSHOT.ru.md).

v0.1050: [Зафиксированы bounded origin ownership, согласованные парные часы, desktop stats и PCM-проверки с открытыми A/V-границами.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1049: [Уточнены origin budget, accepted/pending ownership, точные dequeue категории, desktop diagnostics и десять PCM-режимов.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1048: [Десятый C89-режим показывает bounded origin tracking, точный gated PCM, retry и раздельные accepted/drained категории.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1047: [Проверены host pause, root Stop TAIL dequeue, source phase, BASE resume, GC/native JIT и подтверждённая отмена origin-эпохи.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1046: [Добавлены сериализованные origin diagnostics и точные BASE/TAIL счётчики вместе с FWS/CWS CLI PCM в обоих VM-режимах.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1045: [Origin-учёт включён в root composition; общий ID/bitmap бюджет проверяется до открытия SDL endpoint.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1044: [Проверены 256 эпох с точным PCM, held TAIL, converted stream EOF и согласованными раздельными dequeue-суммами.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1043: [Проверены rollback выделения bitmap и BASE/TAIL retry, наблюдения и seek reset без последующих аллокаций.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1042: [Проверены BASE/TAIL парного конвертированного stream; часы используют фактический допуск BASE без ошибочного callback после write.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1041: [Проверены wraparound, ограничение до render и последние точные кадры эпохи на явно заданных нативных метаданных.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1040: [Проверены TAIL-классификация retry, stop/failed clear, неверный рост FIFO, рекурсия и исключения callbacks.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1039: [Добавлены фиксированный origin bitmap, проверенный dequeue и точные BASE/TAIL счётчики; legacy callbacks сохранены.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1038: [Добавлен точный расчёт памяти BASE/TAIL origin-учёта без изменения смысла прежних predictors.](doc/MIX_ORIGIN_CLOCK.ru.md).

v0.1037: [Зафиксированы finite ownership, owner-frame планирование, полный event drain и проверенные границы tutorial/player.](doc/EVENT_TAILS.ru.md).

v0.1036: [Уточнены pending/release, включение плеера, публикация кадра, mixed-policy совместимость и открытая приёмка.](doc/EVENT_TAILS.ru.md).

v0.1035: [C89-пример показывает конечное исчерпание, retained legacy PCM, pending FIFO и подтверждённый software drain.](doc/EVENT_TAILS.ru.md).

v0.1034: [Проверены host pause, публикация кадра, root Stop, native JIT/GC, точный dequeue drain и release после StopSounds.](doc/EVENT_TAILS.ru.md).

v0.1033: [CLI проверяет одновременные голоса, короткие и многоблочные хвосты, FWS/CWS, native JIT, точный PCM и mute.](doc/EVENT_TAILS.ru.md).

v0.1032: [Подключены finite event pump, resume коротких очередей, завершение после FIFO drain и исключение ожидаемой idle-паузы из underrun.](doc/EVENT_TAILS.ru.md).

v0.1031: [Event-only вывод удерживается при подготовке и callbacks кадра, затем разрешается владельцем в безопасной точке.](doc/EVENT_TAILS.ru.md).

v0.1030: [Добавлены неизменный event policy, CLI finite-audio-tails и совместимые wrappers прежнего mixed-only startup.](doc/EVENT_TAILS.ru.md).

v0.1029: [Проверены 256 эпох с короткими retry, независимым рациональным PCM, pending и истечением queued ID.](doc/EVENT_TAILS.ru.md).

v0.1028: [Проверены NoMultiple до dequeue, последний точный кадр часов и отсутствие аллокаций при коротких retry и отмене.](doc/EVENT_TAILS.ru.md).

v0.1027: [Проверены failed clear/write, исключения, callback reentry и завершение anonymous release после удаления голосов.](doc/EVENT_TAILS.ru.md).

v0.1026: [Добавлено сериализованное наблюдение voice/retry/FIFO с обновлением dequeue clock и сохранением out при ошибке.](doc/EVENT_TAILS.ru.md).

v0.1025: [Добавлен конечный event-only pump с точными последними кадрами и сохранением legacy/release/pending retry.](doc/EVENT_TAILS.ru.md).

v0.1024: [Проверены CLI policy и Boolean-валидация; зафиксированы finite mixer/player/tutorial контракты и открытая приёмка.](doc/FINITE_TAILS.ru.md).

v0.1023: [Уточнены пределы horizon, неделимость retry, source/release, явное включение плеера и открытая reference/device-приёмка.](doc/FINITE_TAILS.ru.md).

v0.1022: [Рациональный PCM-checker проверяет девять режимов, конечный вывод, четыре retry и истечение ID после drain.](doc/FINITE_TAILS.ru.md).

v0.1021: [Девятый C89-режим показывает четыре точных retry, короткий последний TAIL, неизменный BASE и queued/drained ID.](doc/FINITE_TAILS.ru.md).

v0.1020: [Проверены 256 эпох: отклонённый held TAIL, разрешение BASE, точный насыщенный PCM, source EOF и истечение ID.](doc/FINITE_TAILS.ru.md).

v0.1019: [Проверены errors/reentry, сохранение pending после stop/ошибки clear и отсутствие нативных аллокаций после подготовки.](doc/FINITE_TAILS.ru.md).

v0.1018: [CLI проверяет точный короткий WAV при root Stop/gates для FWS/CWS в interpreter/JIT и mute без устройства.](doc/FINITE_TAILS.ru.md).

v0.1017: [Добавлен явный finite mixed-tail startup policy и CLI-флаг до публикации worker; прежний режим остаётся по умолчанию.](doc/FINITE_TAILS.ru.md).

v0.1016: [Проверены strict/converted release, ранний EOF источника и ограниченная арифметика огромных повторов без огромных аллокаций.](doc/FINITE_TAILS.ru.md).

v0.1015: [Добавлен конечный парный gate/EOF-планировщик с общими часами и сохранением владения BASE/TAIL.](doc/FINITE_TAILS.ru.md).

v0.1014: [Добавлен конечный TAIL без нового padding; удерживаемые retry всех прежних режимов остаются неделимыми.](doc/FINITE_TAILS.ru.md).

v0.1013: [Независимо проверены длительности и PCM: четыре source clock, шесть output rate, каналы, повторы и частично прочитанный voice.](doc/FINITE_TAILS.ru.md).

v0.1012: [Добавлен остаток кадров с учётом текущей дробной фазы, lookahead, повторов и release без чтения PCM.](doc/FINITE_TAILS.ru.md).

v0.1011: [Зафиксированы адаптивный FIFO, согласованные часы, точные PCM-проверки и открытая SWF/device-приёмка.](doc/ADAPTIVE_TAILS.ru.md).

v0.1010: [Проверены точный новый padding и смещения сегментов при изменении границ TAIL по однокадровому остатку.](doc/ADAPTIVE_TAILS.ru.md).

v0.1009: [Уточнены размеры по остатку FIFO, неделимый retry, admission-clock, desktop pause и ограничения восьми режимов.](doc/ADAPTIVE_TAILS.ru.md).

v0.1008: [Независимый PCM-оракул проверяет восемь режимов, 14 кадров headroom, retry и queued/drained lifetime ID.](doc/ADAPTIVE_TAILS.ru.md).

v0.1007: [Добавлен C89 headroom-режим: 14 коротких retry, неизменный source cursor и общий drain часов и ID.](doc/ADAPTIVE_TAILS.ru.md).

v0.1006: [Production C89 worker проверен в interpreter/native JIT с GC, коротким retry, точным PCM и возобновлением BASE.](doc/ADAPTIVE_TAILS.ru.md).

v0.1005: [Проверены 256 эпох после удаления VM: FIFO по одному кадру, короткие retry, открытие gate, точный PCM и истечение ID.](doc/ADAPTIVE_TAILS.ru.md).

v0.1004: [После подготовки запрещены все аллокации при коротких блоках, retry, смене gate, остановке событий и seek/clear.](doc/ADAPTIVE_TAILS.ru.md).

v0.1003: [Проверены последний точный кадр, отказ при переполнении, неверный FIFO и подтверждённый сброс эпохи.](doc/ADAPTIVE_TAILS.ru.md).

v0.1002: [Проверены ошибки backend, рекурсия, исключения, запрет BASE и подтверждённая отмена коротких retry.](doc/ADAPTIVE_TAILS.ru.md).

v0.1001: [Парные часы получают наблюдение FIFO непосредственно перед write без ошибочного опроса после принятия.](doc/ADAPTIVE_TAILS.ru.md).

v0.1000: [Парный планировщик EOF/gates заполняет остаток FIFO; короткий TAIL сохраняет приоритет перед новым BASE.](doc/ADAPTIVE_TAILS.ru.md).

v0.999: [Добавлено заполнение остатка FIFO с неделимым retry и сохранением старого API точного размера.](doc/ADAPTIVE_TAILS.ru.md).

v0.998: [Завершены C89/API и desktop-контракты authored Stop, ordered retry, gated PCM и оставшихся reference/device ограничений.](doc/MIXED_GATES.ru.md).

v0.997: [Проверен точный event-only WAV FWS/CWS в обоих VM-режимах при future gate и root Stop, а также mute без аудиоустройства.](doc/MIXED_GATES.ru.md).

v0.996: [Настоящий SDL worker проверен: host pause, authored Stop, GC/native JIT, event-only drain и последующий base resume без rewind.](doc/MIXED_GATES.ru.md).

v0.995: [Добавлен седьмой gated-режим C89: 17 точных кадров, неизменный converter при удержании и failed TAIL-to-BASE переходы.](doc/MIXED_GATES.ru.md).

v0.994: [Удержанный BASE сохранён; проверены drain принятого lead, failed clear и новые события остановленного корня после staged cancellation.](doc/MIXED_GATES.ru.md).

v0.993: [Выполнены 256 native-эпох на четырёх OS-потоках: VM teardown, failed tail retry, смена gates, точный PCM и dequeue/ID.](doc/MIXED_GATES.ru.md).

v0.992: [Проверены seeded границы 2^53, backend/clock rejection и отсутствие выделений в gate/retry/clear после подготовки.](doc/MIXED_GATES.ru.md).

v0.991: [Публикация завершённого кадра отделена от base-разрешения: root Stop держит stream phase, события продолжаются, host pause останавливает очередь.](doc/MIXED_GATES.ru.md).

v0.990: [Mixed worker использует paired gate scheduling: события идут через future gates, retry приоритетен, источники переключаются до event-only подачи.](doc/MIXED_GATES.ru.md).

v0.989: [Проверены TAIL retry перед новым base, удержание BASE, callback-reentry и исключения backend.](doc/MIXED_GATES.ru.md).

v0.988: [Добавлен paired pump_mix_until: разрешённый base или event-only работа при сохранении source position и согласованных часов.](doc/MIXED_GATES.ru.md).

v0.987: [Native stream work вынесен под единый callback barrier; base retry, frame gates и EOF-tail сохранены.](doc/MIXED_GATES.ru.md).

v0.986: [Завершены контракты root-композиции, mixed CLI oracle и continuous ADPCM prefix; зафиксированы проверки и оставшаяся работа.](doc/DESKTOP_MIX.ru.md).

v0.985: [Проверены переходы источников, future gates, rewind-эпохи и точный PCM противоположных голосов в C89 worker после VM teardown.](doc/DESKTOP_MIX.ru.md).

v0.984: [Один predictor используется в preflight/open: отдельный composition-бюджет отвергается до устройства, partial-владение очищается.](doc/DESKTOP_MIX.ru.md).

v0.983: [Base и EOF-tail удерживаются до конца frame callbacks; проверен настоящий SDL worker до и после stream EOF.](doc/DESKTOP_MIX.ru.md).

v0.982: [Проверены pending EOF-tail, пауза без rewind, failed navigation clear и свежие события в новой эпохе.](doc/DESKTOP_MIX.ru.md).

v0.981: [Root-действия проверены interpreter/native JIT: queued NoMultiple, failed clear, GC и новые события после StopSounds.](doc/DESKTOP_MIX.ru.md).

v0.980: [Production C89 refill проверен на явном FIFO: base retry, 5 кадров base+event и 4 кадра хвоста с точными PCM.](doc/DESKTOP_MIX.ru.md).

v0.979: [Worker подаёт согласованные хвосты после финального EOF, учитывает active/pending события и сохраняет StopSounds-остановку stream.](doc/DESKTOP_MIX.ru.md).

v0.978: [Разрешена bounded root-композиция stream/StartSound; навигация согласуется до принятия событий кадра, прежние пути сохранены.](doc/DESKTOP_MIX.ru.md).

v0.977: [Добавлены bounded-владение composition и маршрутизация ID; mixed preflight пока закрыт до интеграционных проверок.](doc/DESKTOP_MIX.ru.md).

v0.976: [Проверены pairing backend, failed/throwing retry, stream-reentry и запрет рассогласованных эпох.](doc/DESKTOP_MIX.ru.md).

v0.975: [Добавлен fvm_swf_stream_output_pump_mix_tail после EOF: точный retry и согласованные счётчики без изменения публичных структур.](doc/DESKTOP_MIX.ru.md).

v0.974: [Описаны tracked stop_all, расчёт памяти, подтверждённый StopSounds и границы проверок.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.973: [Проверено продолжение настоящего stream после stop_all событий и точный retry готового микса.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.972: [Проверены root StopSounds, failed clear/retry, GC и interpreter/JIT; event stop_all сохраняет FIFO.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.971: [Исполняемый C89-пример использует точный бюджет с ID и общую остановку событий.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.970: [Выполнены 256 stop/retry/restart циклов на четырёх OS-потоках с независимыми PCM и ID.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.969: [При нулевом allowance проверены new/new[] для расчёта памяти, нового голоса и stop_all.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.968: [Проверены отмена двух root-ID и независимые сроки новых голосов после принятого PCM.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.967: [Проверены удаление DECLICK-хвостов и новый NoMultiple после сохранённого retry.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.966: [stop_all отвергает callback-reentry; независимый расчёт памяти разрешён внутри callback.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.965: [Проверены точный base/tail retry и новый NoMultiple после сохранённого pending PCM.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.964: [Добавлен точный storage_required_for_tracking без создания владельца и callbacks.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.963: [Добавлен немедленный stop_all событий: pending/FIFO PCM и общий счётчик сохраняются.](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.962: [Завершены контракты идентификаторов очереди и отчёт проверок](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.961: [Проверки C89, пределов, прогресса и release ID](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.960: [Проверки tracked root-событий с таймлайном и JIT](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.959: [Пример queued-ID и bounded adoption на C89](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.958: [Очередные ID независимых OS-владельцев](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.957: [Проверки converted-ID при stream EOF и хвостах](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.956: [Проверки callbacks и tracked-операций без выделений](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.955: [Отмена tracked-ID без изменения pending PCM](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.954: [Наблюдение очередных ID-интервалов смешивания](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.953: [Приём голосов с очередной семантикой NoMultiple](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.952: [Ограниченный optional-учёт ID при смешивании](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.951: [Подготовка защищённого приёма queued-ID](doc/MIX_QUEUE_IDENTITIES.ru.md).

v0.950: [Проверки общего счётчика смешивания](doc/MIX_QUEUE_CLOCK.ru.md).

v0.949: [Проверки жизненного цикла общего счётчика на C89](doc/MIX_QUEUE_CLOCK.ru.md).

v0.948: [Проверки общего отсчёта при смене stream-сегментов](doc/MIX_QUEUE_CLOCK.ru.md).

v0.947: [Пример общего FIFO-счётчика base и tail на C89](doc/MIX_QUEUE_CLOCK.ru.md).

v0.946: [Общие отсчёты независимых OS-владельцев](doc/MIX_QUEUE_CLOCK.ru.md).

v0.945: [Проверки общих отсчётов без выделений](doc/MIX_QUEUE_CLOCK.ru.md).

v0.944: [Проверки общего отсчёта с настоящим таймлайном и JIT](doc/MIX_QUEUE_CLOCK.ru.md).

v0.943: [Общий учёт stream и tail при удалении из FIFO](doc/MIX_QUEUE_CLOCK.ru.md).

v0.942: [Проверки точной границы общего счётчика](doc/MIX_QUEUE_CLOCK.ru.md).

v0.941: [Проверки ошибок callbacks и сброса счётчика](doc/MIX_QUEUE_CLOCK.ru.md).

v0.940: [Завершение общего учёта после опустошения FIFO](doc/MIX_QUEUE_CLOCK.ru.md).

v0.939: [Общий счётчик очереди base и tail](doc/MIX_QUEUE_CLOCK.ru.md).

v0.938: [Проверки native-хвостов смешивания](doc/AUDIO_MIX_TAILS.ru.md).

v0.937: [Проверки stop/release и C89-контрактов tail](doc/AUDIO_MIX_TAILS.ru.md).

v0.936: [Пример точного бюджета и владения tail-адаптера](doc/AUDIO_MIX_TAILS.ru.md).

v0.935: [Пример native tail-pump на C89](doc/AUDIO_MIX_TAILS.ru.md).

v0.934: [Проверки независимых tail-владельцев в OS-потоках](doc/AUDIO_MIX_TAILS.ru.md).

v0.933: [Проверка перехода от настоящего stream EOF к tail](doc/AUDIO_MIX_TAILS.ru.md).

v0.932: [Проверки ограниченного создания и tail без выделений](doc/AUDIO_MIX_TAILS.ru.md).

v0.931: [Проверки callbacks и отмены tail-блоков](doc/AUDIO_MIX_TAILS.ru.md).

v0.930: [Ограничение памяти создания очереди смешивания](doc/AUDIO_MIX_TAILS.ru.md).

v0.929: [Учёт фиксированной памяти очереди смешивания](doc/AUDIO_MIX_TAILS.ru.md).

v0.928: [Наблюдение владельца pending-смешивания](doc/AUDIO_MIX_TAILS.ru.md).

v0.927: [Доигрывание событий через очередь смешивания](doc/AUDIO_MIX_TAILS.ru.md).

v0.926: [Проверки default-табуляции и оставшиеся ограничения](doc/TEXT_DEFAULT_TABS.ru.md).

v0.925: [Проверки выбора face и геометрии default-tabs](doc/TEXT_DEFAULT_TABS.ru.md).

v0.924: [Пример авторазмера и переноса default-tabs](doc/TEXT_DEFAULT_TABS.ru.md).

v0.923: [Пример стандартной табуляции на C89](doc/TEXT_DEFAULT_TABS.ru.md).

v0.922: [Default-табуляция через AVM1 JIT и OS-потоки](doc/TEXT_DEFAULT_TABS.ru.md).

v0.921: [Проверки отката при отказах выделений](doc/TEXT_DEFAULT_TABS.ru.md).

v0.920: [Независимая передача default-геометрии](doc/TEXT_DEFAULT_TABS.ru.md).

v0.919: [Проверки отсутствующих метрик и отката правок](doc/TEXT_DEFAULT_TABS.ru.md).

v0.918: [Перенос при исчерпании предела табуляции](doc/TEXT_DEFAULT_TABS.ru.md).

v0.917: [Стандартная табуляция из метрик шрифта](doc/TEXT_DEFAULT_TABS.ru.md).

v0.916: [Повторяющиеся позиции native-табуляции](doc/TEXT_DEFAULT_TABS.ru.md).

v0.915: [Чтение средней ширины из шрифта](doc/TEXT_DEFAULT_TABS.ru.md).

v0.914: [Границы синтетических tab и результаты регрессионных проверок](doc/TEXT_TABS.ru.md).

v0.913: [C89-пример управляемого tabStops и авторазмера TextField](doc/TEXT_TABS.ru.md).

v0.912: [C89-пример native tabStops с проверкой независимой геометрии](doc/TEXT_TABS.ru.md).

v0.911: [Пользовательские tabStops через AVM1 JIT и независимые native-потоки](doc/TEXT_TABS.ru.md).

v0.910: [Откат текста с tab при семи переборах отказов памяти](doc/TEXT_TABS.ru.md).

v0.909: [Текущие и будущие tabStops через binary/channel-перенос](doc/TEXT_TABS.ru.md).

v0.908: [Getters tabStops, барьеры GC и откат повреждённого состояния](doc/TEXT_TABS.ru.md).

v0.907: [Учёт tabStops в графе и проверка геометрии текста](doc/TEXT_TABS.ru.md).

v0.906: [Собственные таблицы tabStops в геометрии TextFormat и снимках](doc/TEXT_TABS.ru.md).

v0.905: [Shaping слов, интервалы и отступы вокруг пользовательских tab](doc/TEXT_TABS.ru.md).

v0.904: [Переносы при исчерпании tabStops и допуск таблиц](doc/TEXT_TABS.ru.md).

v0.903: [Пользовательские позиции tab в native-раскладке](doc/TEXT_TABS.ru.md).

v0.902: [Контракт отступа первой строки, границы профилей и регрессионные проверки](doc/TEXT_INDENT.ru.md).

v0.901: [C89-пример TextFormat.indent и авторазмера поля](doc/TEXT_INDENT.ru.md).

v0.900: [Исполняемые C89-примеры знакового native-отступа](doc/TEXT_INDENT.ru.md).

v0.899: [Shaping отступов, AVM1 JIT и независимые native-потоки](doc/TEXT_INDENT.ru.md).

v0.898: [Откат отступов при семи переборах отказов памяти](doc/TEXT_INDENT.ru.md).

v0.897: [Перенос отступов и допуск отрицательных значений для Flash8](doc/TEXT_INDENT.ru.md).

v0.896: [Выравнивание, подчёркивание и авторазмер отступов на настоящих начертаниях](doc/TEXT_INDENT.ru.md).

v0.895: [Знаковый TextFormat.indent с транзакционной геометрией и снимками](doc/TEXT_INDENT.ru.md).

v0.894: [Границы абзацев, знаковые отступы и допуск C ABI](doc/TEXT_INDENT.ru.md).

v0.893: [Отступ первой строки абзаца в native-раскладке](doc/TEXT_INDENT.ru.md).

v0.892: [Контракт полей абзаца и результаты регрессионных проверок](doc/TEXT_INSETS.ru.md).

v0.891: [Исполняемый C89-пример полей абзаца и авторазмера](doc/TEXT_INSETS.ru.md).

v0.890: [Поля абзаца через AVM1 JIT и независимые OS-потоки](doc/TEXT_INSETS.ru.md).

v0.889: [Откат полей абзаца при отказах памяти в шести операциях](doc/TEXT_INSETS.ru.md).

v0.888: [Независимый перенос текущих и будущих полей абзаца](doc/TEXT_INSETS.ru.md).

v0.887: [Измерение полей, авторазмер и транзакционные проверки значений](doc/TEXT_INSETS.ru.md).

v0.886: [Геометрия полей при всех выравниваниях и настоящих начертаниях](doc/TEXT_INSETS.ru.md).

v0.885: [Поля абзаца и отступ блока в native TextField](doc/TEXT_INSETS.ru.md).

v0.884: [Контракт и результаты проверок ограниченного выравнивания по ширине](doc/TEXT_JUSTIFY.ru.md).

v0.883: [Исполняемый C89-пример выравнивания по ширине](doc/TEXT_JUSTIFY.ru.md).

v0.882: [Проверки justified-абзацев через AVM1 JIT и native-потоки](doc/TEXT_JUSTIFY.ru.md).

v0.881: [Откат justified layout и состояния поля при отказах памяти](doc/TEXT_JUSTIFY.ru.md).

v0.880: [Проверка допуска justify для Flash8 и упакованного выравнивания](doc/TEXT_JUSTIFY.ru.md).

v0.879: [Выравнивание по ширине при композиции шрифтов и передаче графов](doc/TEXT_JUSTIFY.ru.md).

v0.878: [Пробельные границы и дробный трекинг при выравнивании по ширине](doc/TEXT_JUSTIFY.ru.md).

v0.877: [Ограниченное выравнивание native и AS2 абзацев по ширине](doc/TEXT_JUSTIFY.ru.md).

v0.876: [Контракты явных лиц шрифтов и результаты проверок](doc/TEXT_FONT_VARIANTS.ru.md).

v0.875: [Настоящие лица семейства шрифта в C89-примере текста](doc/TEXT_FONT_VARIANTS.ru.md).

v0.874: [Проверки явных лиц через AVM1 JIT и контексты OS-потоков](doc/TEXT_FONT_VARIANTS.ru.md).

v0.873: [Транзакционная регистрация и изменение начертаний при отказах памяти](doc/TEXT_FONT_VARIANTS.ru.md).

v0.872: [Сохранение выбранных лиц при раскладке и передаче графов](doc/TEXT_FONT_VARIANTS.ru.md).

v0.871: [Проверка выбора начертаний независимыми исходными шрифтами](doc/TEXT_FONT_VARIANTS.ru.md).

v0.870: [Выбор и сохранение настоящих bold и italic лиц семейства](doc/TEXT_FONT_VARIANTS.ru.md).

v0.869: [Явная регистрация начертаний семейства встроенных шрифтов](doc/TEXT_FONT_VARIANTS.ru.md).

v0.868: [Контракты и результаты проверок трекинга необязательных лигатур](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.867: [Демонстрация необязательных лигатур и трекинга fi в C89](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.866: [Трекинг лигатур через native AVM1 JIT и независимые потоки](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.865: [Откат трекинга лигатур на границах выделения памяти](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.864: [Проверки полей с трекингом лигатур через переносы и передачу графов](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.863: [Отдельный шрифт для контекстных и обязательных лигатур](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.862: [Разделимые лигатуры при ненулевом межбуквенном интервале](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.861: [Независимое управление необязательными лигатурами](doc/TEXT_LIGATURE_TRACKING.ru.md).

v0.860: [Контракты горизонтальных опор и результаты native-проверок](doc/TEXT_AUTOSIZE.ru.md).

v0.859: [Восстановление сохранённых опор без повторного субпиксельного сдвига](doc/TEXT_AUTOSIZE.ru.md).

v0.858: [Проверки байтов политики опор и старых заглушек координат](doc/TEXT_AUTOSIZE.ru.md).

v0.857: [Демонстрация опор относительно родителя в C89-примере](doc/TEXT_AUTOSIZE.ru.md).

v0.856: [Проверки горизонтальных опор через AVM1 JIT и OS-потоки](doc/TEXT_AUTOSIZE.ru.md).

v0.855: [Сохранение опор на каждой границе отказа выделения памяти](doc/TEXT_AUTOSIZE.ru.md).

v0.854: [Проверки опор при переносах и владения графом между контекстами](doc/TEXT_AUTOSIZE.ru.md).

v0.853: [Горизонтальные опоры autoSize и перенос координат native-поля](doc/TEXT_AUTOSIZE.ru.md).

v0.852: [Проверки отката предельной геометрии и восстановления авторазмера](doc/TEXT_AUTOSIZE.ru.md).

v0.851: [Проверка восстановленной геометрии и обновление старых нативных accessors](doc/TEXT_AUTOSIZE.ru.md).

v0.850: [Автоматический размер поля в исполняемом C89-примере текста](doc/TEXT_AUTOSIZE.ru.md).

v0.849: [Автоматические размеры в нативном JIT и независимых OS-потоках](doc/TEXT_AUTOSIZE.ru.md).

v0.848: [Проверки атомарности включения авторазмера и правок при отказах памяти](doc/TEXT_AUTOSIZE.ru.md).

v0.847: [Проверки автоматических переносов и независимых текущего/будущего форматов](doc/TEXT_AUTOSIZE.ru.md).

v0.846: [Автоматический размер поля в снимках и передаче графов](doc/TEXT_AUTOSIZE.ru.md).

v0.845: [Атомарный автоматический размер TextField с левым якорем](doc/TEXT_AUTOSIZE.ru.md).

v0.844: [Сохранение nonzero-заливки при пересечении подчёркивания и глифов](doc/TEXT_UNDERLINE.ru.md).

v0.843: [Независимые контуры подчёркивания и C89-проверка пикселя растеризации](doc/TEXT_UNDERLINE.ru.md).

v0.842: [Геометрия подчёркивания в нативном JIT и конкурентном чтении раскладки](doc/TEXT_UNDERLINE.ru.md).

v0.841: [Отказ непригодных метрик подчёркивания и проверки атомарности при OOM](doc/TEXT_UNDERLINE.ru.md).

v0.840: [Проверки передачи подчёркивания и выравнивания с переносами и без](doc/TEXT_UNDERLINE.ru.md).

v0.839: [Отрисовка и передача однородного подчёркивания TextFormat AS2](doc/TEXT_UNDERLINE.ru.md).

v0.838: [Независимые контуры подчёркивания строк из готовой раскладки](doc/TEXT_UNDERLINE.ru.md).

v0.837: [Масштабированные метрики подчёркивания без изменения геометрии глифов](doc/TEXT_UNDERLINE.ru.md).

v0.836: [Проверка восстановления поля с интервалом и метрик TextFormat](doc/TEXT_SPACING.ru.md).

v0.835: [Знаковые интервалы в C89-примере растеризации текста](doc/TEXT_SPACING.ru.md).

v0.834: [Интервалы в нативном AVM1 JIT и независимых потоках шрифтов](doc/TEXT_SPACING.ru.md).

v0.833: [Проверки атомарности правок и восстановления раскладки после отказов памяти](doc/TEXT_SPACING.ru.md).

v0.832: [Проверки передачи интервалов и границ bidi-раскладки](doc/TEXT_SPACING.ru.md).

v0.831: [Знаковый интервал TextFormat и сохранение при передаче поля](doc/TEXT_SPACING.ru.md).

v0.830: [Нативный знаковый интервал с согласованными переносами и контурами](doc/TEXT_SPACING.ru.md).

v0.829: [Проверки независимых bidi-потоков и входа в разных профилях](doc/FONT_LIBRARIES.ru.md).

v0.828: [Проверки отказов памяти bidi и повторной перестановки на месте](doc/FONT_LIBRARIES.ru.md).

v0.827: [Проверка C-входа bidi до чтения enum и публикации результатов](doc/FONT_LIBRARIES.ru.md).

v0.826: [Проверка direction нативного shaping и изоляция features между потоками](doc/TEXT_KERNING.ru.md).

v0.825: [Нативное выполнение кернинга VM и транзакционный допуск форматов](doc/TEXT_KERNING.ru.md).

v0.824: [Явный выбор кернинга в растровом текстовом примере C89](doc/TEXT_KERNING.ru.md).

v0.823: [Сохранение текущего и будущего кернинга в переносимых записях поля](doc/TEXT_KERNING.ru.md).

v0.822: [Кернинг AS2 в текущем и будущем стилях TextField](doc/TEXT_KERNING.ru.md).

v0.821: [Передача кернинга в выбор строк и размещение глифов](doc/TEXT_KERNING.ru.md).

v0.820: [Явное нативное управление парным кернингом](doc/TEXT_KERNING.ru.md).

v0.819: [Конкурентное нативное декодирование защищённого MP3 с атомарным повтором](doc/MP3_CRC.ru.md).

v0.818: [Ограниченная проверка аудио C89 без создания выходного файла](doc/MP3_CRC.ru.md).

v0.817: [Независимые оракулы защищённого MP3 и отказов до открытия аудио](doc/MP3_CRC.ru.md).

v0.816: [Проверка корневых MP3 до открытия настольного аудио](doc/MP3_CRC.ru.md).

v0.815: [Проверка CRC free-format блоков до фиксации геометрии](doc/MP3_CRC.ru.md).

v0.814: [Отклонение ошибок CRC до изменения декодера и PCM](doc/MP3_CRC.ru.md).

v0.813: [Проверка CRC защищённых областей MPEG Layer-III без выделений](doc/MP3_CRC.ru.md).

v0.812: [Воспроизводимая генерация free-format с независимыми PCM-эталонами](doc/FREE_FORMAT_MP3.ru.md).

v0.811: [Независимые оракулы SDL для free-format потоков и смешанных событий](doc/FREE_FORMAT_MP3.ru.md).

v0.810: [Ограниченный экспорт подготовленного звука реальных SWF через C89](doc/FREE_FORMAT_MP3.ru.md).

v0.809: [Ограничение определения free-format длины точными счётчиками](doc/FREE_FORMAT_MP3.ru.md).

v0.808: [Подготовка free-format потоковых блоков по точному числу отсчётов](doc/FREE_FORMAT_MP3.ru.md).

v0.807: [Декодирование free-format DefineSound с сохранением длины пакетов](doc/FREE_FORMAT_MP3.ru.md).

v0.806: [Определение длины free-format MPEG по последовательным заголовкам](doc/FREE_FORMAT_MP3.ru.md).

v0.805: [Непрерывное преобразование в экспорте C89 и нативных потоках](doc/STREAM_PCM_CONVERSION.ru.md).

v0.804: [Непрерывный потоковый звук в настольном плеере](doc/STREAM_PCM_CONVERSION.ru.md).

v0.803: [Непрерывная интерполяция подготовленного потока](doc/STREAM_PCM_CONVERSION.ru.md).

v0.802: [Скопированные границы ID очереди для нативной диагностики](doc/QUEUED_SOUND_IDS.ru.md).

v0.801: [Учебный C89-хост с идентичностью звука до конца FIFO](doc/QUEUED_SOUND_IDS.ru.md).

v0.800: [Проверка отката памяти ID и владения при отказах backend](doc/QUEUED_SOUND_IDS.ru.md).

v0.799: [Проверка C89-управления ID и защиты от повторного входа callback](doc/QUEUED_SOUND_IDS.ru.md).

v0.798: [Проверка ID короткого звука через реальные кадры и управление](doc/QUEUED_SOUND_IDS.ru.md).

v0.797: [Удержание desktop SyncNoMultiple до конца отсчётов очереди](doc/QUEUED_SOUND_IDS.ru.md).

v0.796: [Отмена идентичностей после подтверждённой остановки очереди](doc/QUEUED_SOUND_IDS.ru.md).

v0.795: [Передача голосов с учётом SyncNoMultiple до конца очереди](doc/QUEUED_SOUND_IDS.ru.md).

v0.794: [Необязательная таблица идентичностей очереди и учёт памяти выхода](doc/QUEUED_SOUND_IDS.ru.md).

v0.793: [Подсчёт оставшихся кадров голоса на новой выходной сетке](doc/QUEUED_SOUND_IDS.ru.md).

v0.792: [Проверка отказов подготовки и регрессий владения в плеере](doc/NATIVE_VOICES.ru.md).

v0.791: [Проверка смешивания трёх кодеков по независимому эталону PCM](doc/NATIVE_VOICES.ru.md).

v0.790: [Повторное воспроизведение шаблонов в нативном примере C89](doc/NATIVE_VOICES.ru.md).

v0.789: [Проверка памяти микшера до выделения объекта и слотов](doc/NATIVE_VOICES.ru.md).

v0.788: [Изоляция подготовки и четыре независимых потока голосов](doc/NATIVE_VOICES.ru.md).

v0.787: [Подготовка событийного PCM вне mutex с общим бюджетом памяти](doc/NATIVE_VOICES.ru.md).

v0.786: [Общий бюджет памяти микшера и запрос активности звуков](doc/NATIVE_VOICES.ru.md).

v0.785: [Перемещение по голосам с восстановлением времени огибающей](doc/NATIVE_VOICES.ru.md).

v0.784: [Копирование подготовленных голосов без повторного декодирования](doc/NATIVE_VOICES.ru.md).

v0.783: [Ограничение подготовки голосов и точный учёт памяти](doc/NATIVE_VOICES.ru.md).

v0.782: [Передача prepared voices и stop sound ID без обращения к VM](doc/MP3_AUDIO.ru.md).

v0.781: [C89 экспорт MP3 voices/streams после полного VM teardown](doc/MP3_AUDIO.ru.md).

v0.780: [Воспроизведение MP3 root streams через native SDL queue](doc/MP3_AUDIO.ru.md).

v0.779: [Bounded MP3 streams с общим reservoir и однократной latency](doc/MP3_AUDIO.ru.md).

v0.778: [MP3 event styles: проверяемые позиции на фактических MPEG rates](doc/MP3_AUDIO.ru.md).

v0.777: [MP3 DefineSound: диапазоны, явная latency и фактическая MPEG geometry](doc/MP3_AUDIO.ru.md).

v0.776: [Зафиксированный scalar MPEG Layer-III decoder без allocation](doc/MP3_AUDIO.ru.md).

v0.775: [C89 экспорт неполных ADPCM после VM teardown: prepared и encoded ranges](examples/multimedia/swf_stream_adpcm/README.md).

v0.774: [Чтение диапазонов ADPCM без выделения памяти](doc/STREAM_ADPCM_COUNTED.ru.md#диапазоны-без-выделения-памяти-v0774).

v0.773: [Подготовка неполных ADPCM-пакетов с явными длинами](doc/STREAM_ADPCM_COUNTED.ru.md).

v0.772: [Сохранение смешанного PCM при отказах аудиоочереди](doc/MIX_QUEUE.ru.md).

v0.771: [Смешивание событийных голосов с потоковым PCM до единственного насыщения](doc/STREAM_EVENT_MIX.ru.md).

v0.770: [Кросс-сборка Windows x64 плеера через Zig и целевую статическую SDL](doc/CROSS_WINDOWS_ZIG.ru.md).

v0.769: [Повторное наполнение потоковой аудиоочереди после наблюдаемого опустошения](doc/PLAYER_STREAM_RECOVERY.ru.md).

v0.768: [Ограничение памяти снимков исходных аудиоблоков без декодирования](doc/STREAM_STORAGE.ru.md).

v0.767: [Общий предел памяти подготовленных потоков до старта плеера](doc/PLAYER_STREAM_BUDGETS.ru.md).

v0.766: [Явный бюджет подготовки снимков и наблюдение нативной памяти](doc/STREAM_STORAGE.ru.md).

v0.765: [Завершение разрешённого потокового звука перед конечным выходом плеера](doc/PLAYER_PCM_STREAMS.ru.md).

v0.764: [Воспроизведение целых потоковых ADPCM-пакетов через SDL-очередь](doc/PLAYER_PCM_STREAMS.ru.md).

v0.763: [Подготовка независимого PCM из целых потоковых ADPCM-пакетов](doc/STREAM_ADPCM.ru.md).

v0.762: [Кросс-сборка полного ядра Linux/musl и SDL-плеера](doc/CROSS_LINUX_ZIG.ru.md).

v0.761: [Смена PCM-заголовков и формата через одну SDL-очередь](doc/PLAYER_PCM_STREAMS.ru.md).

v0.759: [Отслеживать goto на тот же кадр и сбрасывать старый PCM](doc/PLAYER_PCM_STREAMS.ru.md).

v0.758: [Один корневой PCM-поток через SDL-очередь с разрешением по кадрам](doc/PLAYER_PCM_STREAMS.ru.md).

v0.756: [Drive native stream frame gates from an actual MovieClip](doc/STREAM_PCM_OUTPUT.ru.md).

v0.755: [Authorize native stream output by actual movie frame](doc/STREAM_PCM_OUTPUT.ru.md).

v0.754: [Bounded native FIFO and explicit file consumer for stream PCM](doc/STREAM_PCM_OUTPUT.ru.md).

v0.753: [Caller-buffered prepared stream queue producer](doc/STREAM_PCM_OUTPUT.ru.md).

v0.752: [Export converted native stream PCM after source teardown](doc/STREAM_PCM_CONVERSION.ru.md).

v0.751: [Explicit prepared stream PCM conversion](doc/STREAM_PCM_CONVERSION.ru.md).

v0.750: [Native stream PCM export after VM teardown](doc/STREAM_PCM_READER.ru.md).

v0.749: [Frame-aware prepared PCM reader](doc/STREAM_PCM_READER.ru.md).

v0.748: [Verified receiver measurements drive actual player profile changes](doc/RECEIVE_WINDOWS.ru.md).

v0.747: [Unique-part receive windows with frozen feedback](doc/RECEIVE_WINDOWS.ru.md).

v0.746: [Connect explicit host feedback to actual player frame export](doc/CHANNEL_ADAPTATION.ru.md).

v0.745: [Выбор профиля по статистике канала](doc/CHANNEL_ADAPTATION.ru.md).

v0.744: [Настоящие кадры, смена профиля и потери канала](doc/PLAYER_FRAME_PARTS.ru.md).

v0.743: [Части кадров плеера с согласованной геометрией](doc/PLAYER_FRAME_PARTS.ru.md).

v0.742: [Согласование размеров через служебный tag 0](doc/MEDIA_SLOTS.ru.md).

v0.741: [Приём частей файлов и потокового медиа](doc/MEDIA_SLOTS.ru.md).

v0.740: [Фиксированные слоты S-записей](doc/MEDIA_SLOTS.ru.md).

v0.739: [Кадры с согласованным msize](doc/PLAYER_RESOURCE_FRAMES.ru.md#согласованный-msize-v0739).

v0.738: [Области закреплённых кадров](doc/PLAYER_RESOURCE_FRAMES.ru.md#области-кадра-v0738).

v0.737: [Frame pixels over 9P](doc/PLAYER_RESOURCE_FRAMES.md#live-9p-pixels-v0737) / [Пиксели через 9P](doc/PLAYER_RESOURCE_FRAMES.ru.md#пиксели-через-9p-v0737).

v0.736: [Leased resource frames](doc/PLAYER_RESOURCE_FRAMES.md) / [Кадры ресурсов](doc/PLAYER_RESOURCE_FRAMES.ru.md).

v0.735: [Live player through 9P](doc/PLAYER_RESOURCE_STATE.md#live-9p-player-v0735) / [Плеер через 9P](doc/PLAYER_RESOURCE_STATE.ru.md#плеер-через-9p-v0735).

v0.734: [Copied player state](doc/PLAYER_RESOURCE_STATE.md) / [Состояние плеера](doc/PLAYER_RESOURCE_STATE.ru.md).

v0.733: [9P2000 resource adapter](doc/RESOURCE_9P.md) / [Адаптер 9P2000](doc/RESOURCE_9P.ru.md).

v0.732: [Portable resource files](doc/RESOURCE_FILES.md) / [Файлы ресурсов](doc/RESOURCE_FILES.ru.md).

v0.731: [Hybrid UDP/TCP policy](doc/HYBRID_HEARTBEAT.md) / [Гибридная политика](doc/HYBRID_HEARTBEAT.ru.md).

v0.640: [Canonical sound frames](doc/SOUND_SNAPSHOT.md#canonical-frame-binding-v0640) / [Канонические звуковые кадры](doc/SOUND_SNAPSHOT.ru.md#канонический-кадр-v0640).

v0.639: [Sound metadata work](doc/SOUND_SNAPSHOT.md#metadata-work-v0639) / [Бюджет звуковых metadata](doc/SOUND_SNAPSHOT.ru.md#бюджет-метаданных-v0639).

v0.638: [Root sound work](doc/SOUND_SNAPSHOT.md#delivery-work-v0638) / [Бюджет доставки звука](doc/SOUND_SNAPSHOT.ru.md#бюджет-доставки-v0638).

v0.637: [Sound ordinal snapshots](doc/SOUND_SNAPSHOT.md) / [Снимки звуковых событий](doc/SOUND_SNAPSHOT.ru.md).

v0.636: [Scene graph safety](doc/SCENE_CONTROLS.md#graph-validation-and-legacy-depths-v0636) / [Граф сцены и старые глубины](doc/SCENE_CONTROLS.ru.md#граф-сцены-и-старые-глубины-v0636).

v0.635: [Scene traversal work](doc/SCENE_CONTROLS.md#scene-work-v0635) / [Бюджет сцены](doc/SCENE_CONTROLS.ru.md#бюджет-сцены-v0635).

v0.634: [Scene control snapshots](doc/SCENE_CONTROLS.md) / [Снимки управляющих записей](doc/SCENE_CONTROLS.ru.md).

v0.633: [Platform source evidence](doc/PLATFORM_VALIDATION.md#source-stability-v0633) / [Исходники при приёмке](doc/PLATFORM_VALIDATION.ru.md#стабильность-исходников-v0633).

v0.632: [Initializer progress](doc/INITIALIZER_PROGRESS.md) / [Бюджет инициализации](doc/INITIALIZER_PROGRESS.ru.md).

v0.631: [Frame action snapshots](doc/FRAME_BLOCKS.md) / [Блоки действий кадра](doc/FRAME_BLOCKS.ru.md).

v0.630: [Raster depth snapshots](doc/RASTER_DEPTH_ORDER.md) / [Отрисовка по глубинам](doc/RASTER_DEPTH_ORDER.ru.md).

v0.629: [Timeline depth order](doc/TIMELINE_DEPTH_ORDER.md) / [Порядок tick-обхода](doc/TIMELINE_DEPTH_ORDER.ru.md).

v0.628: [ClipEventKeyPress](doc/CLIP_KEYPRESS.md) / [Коды и доставка KeyPress](doc/CLIP_KEYPRESS.ru.md).

v0.627: [MovieClip input methods](doc/CLIP_INPUT_METHODS.md) / [Методы ввода MovieClip](doc/CLIP_INPUT_METHODS.ru.md).

v0.626: [Clip input broadcasts](doc/CLIP_INPUT.md) / [Ввод для дерева клипов](doc/CLIP_INPUT.ru.md), [C89](examples/embedding/clip_actions/README.md).

v0.625: [Retained ClipActions](doc/CLIP_ACTIONS.md) / [Обработчики ClipActions](doc/CLIP_ACTIONS.ru.md), [C89](examples/embedding/clip_actions/README.md).

v0.624: [MovieClip ticks](doc/MOVIECLIP_TICKS.md) / [Общий бюджет ticks](doc/MOVIECLIP_TICKS.ru.md), [C89](examples/embedding/movieclip/README.md).

v0.623: [updateAfterEvent and redraw requests](doc/UPDATE_AFTER_EVENT.md) / [Внеочередная отрисовка](doc/UPDATE_AFTER_EVENT.ru.md), [C89](examples/embedding/timers/README.md).

v0.622: [Timer wakeups](doc/TIMER_WAKEUP.md) / [Пробуждение таймеров](doc/TIMER_WAKEUP.ru.md), [C89](examples/embedding/timers/README.md).

v0.621: [Scheduled callbacks](doc/TIMERS.md) / [Таймеры](doc/TIMERS.ru.md), [C89](examples/embedding/timers/README.md).

v0.620: [getTimer global](doc/GET_TIMER.md) / [Глобальная getTimer](doc/GET_TIMER.ru.md), [C89](examples/embedding/get_timer/README.md).

v0.619: [AS1/AS2 URL globals and trusted global-call roots](doc/URL_FUNCTIONS.md) / [URL-функции и корни глобальных вызовов](doc/URL_FUNCTIONS.ru.md), [C89](examples/embedding/url_encoding/README.md).

v0.618: [Shared input event budgets and worker cancellation](doc/INPUT_EVENT_BUDGET.md) / [общий бюджет событий и остановка worker](doc/INPUT_EVENT_BUDGET.ru.md).

v0.617: [Bounded broadcaster registration](doc/AS_BROADCASTER.md#registration-work-limits-v0617) / [бюджет регистрации слушателей](doc/AS_BROADCASTER.ru.md#ограничение-работы-регистрации-v0617).

v0.616: [Broadcaster transports and player integration](doc/AS_BROADCASTER.md#transport-and-player-v0616) / [каналы и плеер](doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616), [two-context C89 tutorial](examples/embedding/broadcaster/README.md).

v0.615: [AsBroadcaster: generic application events](doc/AS_BROADCASTER.md) / [пользовательские события](doc/AS_BROADCASTER.ru.md), [C89/AS tutorial](examples/embedding/broadcaster/README.md).

v0.581: API очереди PCM с повтором записи, часы потребления и настоящий
SDL2-вывод при явном включении SDK. [Руководство](doc/SWF_SOUND_OUTPUT.ru.md) /
[EN](doc/SWF_SOUND_OUTPUT.md), [учебный пример C89](examples/multimedia/swf_audio_queue/README.md).
Часы программной FIFO, не ЦАП; автоматическая desktop A/V синхронизация ещё открыта.

Тестовые плееры v0.582: [Linux x64, Windows x64, Android ARM64](doc/PLAYER_TEST_BUILDS.ru.md)
и [три готовых SWF](examples/player/animation_audio/README.md).

v0.584: [Android PCM/WAV diagnostics / диагностика звука](doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md).

v0.586: [согласование блока SDL и запаса очереди](doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md#v0586-публикация-согласованного-блока-устройства).

v0.587: [optional stop release / затухание остановки](doc/PLAYER_AUDIO_RELEASE.md).

v0.588: [ADPCM test-tone block restart fix / исправление тестового ADPCM-тона](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0588-adpcm-fixture-block-restarts).

v0.589: [Опрос Key и клавиатурные снимки хоста](doc/KEYBOARD.ru.md).

v0.610: [Native cursor application in SDL2, GTK3 and Qt5 / курсор во фронтендах](examples/frontends/README.md).

v0.609: [Mouse.hide/show and SWF5–8 cursor requests / видимость курсора из скрипта](doc/MOUSE.md#script-hide-and-show).

v0.608: [Context-owned cursor visibility / запрос видимости курсора](doc/MOUSE.md#cursor-visibility-request).

v0.607: [Native wheel normalization in SDL2, GTK3, Qt5 and evdev / колесо во фронтендах](examples/frontends/README.md).

v0.606: [Wheel events in the mixed FIFO / колесо в общей очереди](doc/INPUT_QUEUE.md), SWF6–8 interpreter/JIT checks.

v0.605: [Wheel listener delivery / доставка onMouseWheel](doc/MOUSE.md#wheel-events), [C89 tutorial](examples/embedding/mouse_wheel/README.md).

v0.604: [GTK3, Qt5 and Linux framebuffer/input/V4L2 frontends / новые фронтенды](examples/frontends/README.md).

v0.603: [Mixed gesture/Mouse/Key FIFO and bounded cancellation / общая очередь ввода](doc/INPUT_QUEUE.md), [C89 tutorial](examples/embedding/mixed_input/README.md).

v0.602: [Separate patch for the Clutter3D library / отдельный патч библиотеки](examples/frontends/clutter3d/LIBRARY_PATCH.md).

v0.601: [Clutter fork subpixel inverse and Linux EGL/GLES2 SDK / дробные координаты и EGL/GLES2](external/clutter3d/README.md#linux-eglgles2-profile-v0601).

v0.600: [Frontend tutorials / примеры фронтендов](examples/frontends/README.md): SDL2, selected Clutter/Cogl fork, relocated headless host; [SDK source and fixes](external/clutter3d/README.md).

v0.599: [Runnable host without SDL / запускаемый хост без SDL](examples/frontends/headless/README.md); shared pointer projection in the SDL FIFO.

v0.598: [Independent frame/event host boundary / независимый хост кадров и событий](doc/HOST_BACKENDS.md) ([RU](doc/HOST_BACKENDS.ru.md)).

v0.597: [Muted sound-bearing movie playback / запуск звуковых роликов с --mute](doc/PLAYER_POINTER.md#muted-movie-playback) ([RU](doc/PLAYER_POINTER.ru.md#ролики-без-звука)).

v0.596: [Player primary touch and shared input FIFO / касания и общая очередь ввода](doc/PLAYER_POINTER.md) ([RU](doc/PLAYER_POINTER.ru.md)).

v0.595: [Primary-pointer Mouse listeners / события касания и указателя](doc/MOUSE.md) ([RU](doc/MOUSE.ru.md)).

v0.594: [Selection observers / наблюдатели смены фокуса](doc/SELECTION.md#selection-listeners) ([RU](doc/SELECTION.ru.md#слушатели-selection)).

v0.593: [MovieClip focus notifications / уведомления смены фокуса](doc/SELECTION.md#focus-notifications) ([RU](doc/SELECTION.ru.md#уведомления-фокуса)).

v0.592: [Selection MovieClip subset / скриптовый выбор клипа](doc/SELECTION.md) ([RU](doc/SELECTION.ru.md)).

v0.591: [Explicit host keyboard focus / выбор клипа для клавиатуры](doc/KEYBOARD_FOCUS.md) ([RU](doc/KEYBOARD_FOCUS.ru.md)).

v0.590: [Key listeners and ordered transitions / слушатели и переходы](doc/KEYBOARD_LISTENERS.md) ([RU](doc/KEYBOARD_LISTENERS.ru.md)).

## Просмотр документации

Из корня проекта установите зависимости и запустите локальный просмотрщик:

```sh
python -m pip install -r doku_viewer/requirements.txt
python view_docs.py
```

[Руководство просмотрщика](doku_viewer/README.md). Python 2.7.9 / Python 3.


Повторяемые AGG/Cairo/minimal и адаптер Android CTest/ADB:
[проверка платформ](doc/PLATFORM_VALIDATION.ru.md). Реальные запуски остальных
ОС и завершение стадий 1–9 пока не подтверждены.

В JIT добавлены запись/объявление переменных, свойства и удаление:
[контракты и проверки](doc/JIT_REFERENCES.ru.md).
Float/double и семь унарных операций работают в явном и автоматическом JIT:
[числовые контракты и проверки](doc/JIT_NUMBERS.ru.md).
Регистры JIT используют готовые банки Function2/контекста/корневого блока:
[контракты и проверки](doc/JIT_REGISTERS.ru.md).
ConstantPool и индексированные Push сохраняют захваченные пулы и выбор по ветвям;
[контракты и проверки](doc/JIT_POOLS.ru.md).
CallMethod/NewMethod используют общий нативный сервис с корнями GC;
[контракты и проверки](doc/JIT_METHODS.ru.md).
CallFunction/NewObject сохраняют лексический и версионный поиск имён;
[контракты и проверки](doc/JIT_NAMED_CALLS.ru.md).
SetTarget/SetTarget2 выбирают временную шкалу с восстановлением цели;
[контракты и проверки](doc/JIT_TARGETS.ru.md).
Двенадцать строковых Action используют общие версионные сервисы JIT;
[контракты и проверки](doc/JIT_STRINGS.ru.md).
InitArray/InitObject сохраняют версионные правила счётчика и элементов;
[контракты и проверки](doc/JIT_LITERALS.ru.md).
Equals/Less и And/Or используют общие версионные преобразования и hook;
[контракты и проверки](doc/JIT_LEGACY.ru.md).
Extends/Implements/InstanceOf/Cast используют общие сервисы с корнями GC;
[контракты и проверки](doc/JIT_CLASSES.ru.md).
GetProperty/SetProperty/TargetPath используют общие нативные сервисы сцены;
[контракты и проверки](doc/JIT_DISPLAY.ru.md).
Нативные Trace/Throw/GetTime/RandomNumber: [контракты и проверки](doc/JIT_EFFECTS.ru.md).
Enumerate/Enumerate2: [контракты и проверки](doc/JIT_ENUMERATE.ru.md).
Нативные команды таймлайна: [контракты и проверки](doc/JIT_TIMELINE.ru.md).
CloneSprite/RemoveSprite/StartDrag/EndDrag: [контракты и проверки](doc/JIT_SCENE.ru.md).
WaitForFrame/WaitForFrame2: [контракты и проверки](doc/JIT_WAIT_FRAMES.ru.md).
GetURL/GetURL2/Call и JIT кадров: [контракты и проверки](doc/JIT_DISPATCH.ru.md).

Контрольный коммит зависимостей: `extern/` объединена с `external/`.
Включены исходники и настройки Cairo 1.16.0, Pixman 0.38.4 и SQLite 3.50.4.
Конкурирующие варианты выбираются в корневом [DEFS.txt](DEFS.txt); по умолчанию
AGG + SQLite + HEADLESS. См. [ключи сборки](doc/BUILD_DEFINITIONS.ru.md) и
[проверки](doc/DEPENDENCY_CHECKPOINT.ru.md). Экранные ключи пока зарезервированы.

Этап стандартной числовой библиотеки: добавлены Math и преобразования
примитивов. См. [числовой API](doc/STANDARD_NUMERIC.ru.md) для установки,
проверенного поведения и ограничений. Историческая контрольная точка: 95 Linux-тестов и примеров прошли.
Текущая команда пользователя на обновление зеркала указана выше.

Этап разработки SWF 8: загрузка FWS/CWS с лимитами, все типы AVM1 Push,
пулы/4 регистра, C callbacks по образцу Lua, исходники zlib/AGG.
VM пока частичная. Доступен явный экспериментальный JIT для подмножества AVM1
на Linux x86-64; полного проигрывателя пока нет. См. [JIT](doc/JIT.md),
[план совместимости](doc/SWF8_ROADMAP.ru.md), [загрузку](doc/SWF_LOADING.ru.md),
[API расширений](doc/LUA_STYLE_API.ru.md), [сборку](doc/BUILD_PLATFORMS.ru.md).

# FlashVM — документация на русском языке

FlashVM — экспериментальная библиотека C++98 с публичным C89 API для SWF 1–8 и частичной реализации AVM1. **Это не готовый Flash Player.**

## Сборка

Требуются CMake, компиляторы C89 и C++98. Команды: `cmake -S . -B build`, `cmake --build build`, `ctest --test-dir build --output-on-failure`.

## Состав API

- `flashvm/flashvm.h`: конфигурация, Runtime, Context, загрузка SWF из памяти. [`fvm_advance`](include/flashvm/flashvm.h#L74) пока возвращает [`FVM_UNSUPPORTED`](include/flashvm/flashvm.h#L46).
- `flashvm/value.h`: значения, объекты, глобальные переменные, нативные функции; объекты принадлежат контексту, указатели не сохранять после его уничтожения.
- `flashvm/swf.h`: анализ FWS/CWS до SWF 8, без ZWS.
- `flashvm/avm1.h`: анализ и частичное выполнение AVM1; JIT отсутствует.
- `flashvm/input.h`: правила сопоставления жестов с действиями мыши, C-обработчик событий. Подробности на русском и английском: [INPUT_EVENTS.md](doc/INPUT_EVENTS.md).
- `flashvm/sqlite.h`: необязательный адаптер SQLite, включается ключом SQLITE из DEFS.txt; пока не интегрирован с ActionScript.

## Примеры

Каталоги [examples/embedding](examples/embedding) и [examples/extending](examples/extending) содержат примеры C и AS2. В README каждой подпапки указано, какие функции работают, а какие только запланированы. Новый [gesture_dispatch](examples/embedding/gesture_dispatch/README.md) показывает тап двумя пальцами и сопоставление с правой кнопкой мыши.

## Ограничения и безопасность

Нет полной совместимости Flash Player 8, JIT, рендеринга, аудио/видео и полноценного диспетчера событий AVM1. Не запускайте недоверенные SWF как изолированные сценарии: песочница не реализована. Поддержка C89/C++98 относится к исходникам FlashVM; сторонние библиотеки могут требовать более новые инструменты.

## Документация API

Подробные документы на английском находятся в `doc/`. Русский обзор доступен в этом файле; двуязычная спецификация ввода — в `doc/INPUT_EVENTS.md`. Для остальных документов добавлены русские сопроводительные файлы `.ru.md` с описанием возможностей и ограничений; подробные сигнатуры остаются в английских оригиналах.

## Потоки отрисовки

Добавлен переносимый исполнитель заданий отрисовки через переходники
`ports/windows`, `ports/linux`, `ports/mac`, `ports/android`.
[Описание API на русском и английском](doc/RENDER_THREADS.md),
[пример на C89](examples/embedding/parallel_render/README.md).
Пока это механизм задач хоста, а не готовый рендерер MovieClip.

## Платформенные адаптеры ввода и сети

Подробности: [API платформенных адаптеров](doc/OS_PORTS.ru.md) и [English](doc/OS_PORTS.md). Реализации: `ports/{windows,linux,mac,android}`, пример на C89: `examples/embedding/os_ports`. Сеть пока блокирующая и не доступна напрямую из ActionScript; события ввода ещё не вызывают обработчики Flash автоматически.

## Этап 14: распознавание касаний

`flashvm/gesture_recognizer.h` распознаёт тапы, мультитапы и свайпы из
событий приложения-хоста с настраиваемыми порогами. Подробности:
`doc/GESTURE_RECOGNIZER.md`, пример:
`examples/embedding/gesture_recognizer/`. Пока события поступают в C callback,
а не в обработчики объектов AVM1.

### Отложенные события ввода (этап 15)

См. [документацию очереди ввода (RU/EN)](doc/INPUT_QUEUE.md) и
[`examples/embedding/input_queue`](examples/embedding/input_queue).
Очередь предназначена для **одного потока** и пока не вызывает обработчики
ActionScript у MovieClip. Межпоточная передача событий остаётся задачей хоста.

- [Нативные методы AVM1 / Native methods](doc/AVM1_METHODS.md)
- [Пример методов объектов на C89](examples/embedding/object_methods/README.md)

## Этап 17: вызовы скриптовых методов

Добавлена начальная поддержка `CallMethod` для именованных функций AVM1 с
параметрами и привязкой `this` к объекту-получателю. Подробности, ограничения
и пример C89/AS: [документация RU/EN](doc/AVM1_SCRIPT_METHODS.md),
[пример](examples/embedding/script_methods/README.md).
Функции как свойства объектов пока не поддерживаются.

## Инспекция таблиц (постепенная реализация)

См. `doc/TABLES_DEBUG.md` и `examples/embedding/tables_debug/`: API таблиц и начальной инспекции отладчика на C89. Окружения функций в стиле Lua, полный протокол метаметодов и пошаговое выполнение **пока не реализованы**.

### Собственные контейнеры и метатаблицы

Описание реализации C++98, ограничений и пример на C89 находятся в
[документации контейнеров и метатаблиц](doc/CUSTOM_CONTAINERS.md).

### Собственные внутренние контейнеры

В ядре C++98 вместо `std::string`, `std::vector` и `std::map` используются собственные реализации из `src/details/`. Подробности, ограничения, текущее состояние гибридных таблиц и метатаблиц, примеры: [документация на русском](doc/INTERNAL_CONTAINERS.ru.md) и [английская версия](doc/INTERNAL_CONTAINERS.md).

## Переносимые текстовые функции (этап 21)

См. [Текстовый API](doc/TEXT_API.md): проверка, чтение и запись UTF-8,
смещения символов и ограниченный движок регулярных выражений по байтам.
Рабочий пример находится в `examples/embedding/text/`. Исходники движка
регулярных выражений — в `external/`. Профили встроенных графических библиотек и SQLite
описаны в [external/README.md](external/README.md).

### Внешние зависимости и гибридная таблица

Состояние включения исходников библиотек, ограничения автономной сборки и
устройство нового контейнера описаны в
[документации](doc/EXTERNAL_DEPENDENCIES.ru.md).

### Этап 23: нативные метаметоды

См. [описание API](doc/METAMETHODS.ru.md) и `tests/metamethods.c`:
`__index`, `__newindex`, `__iterate` и явная финализация `__gc`.
Автоматического сборщика мусора и синтаксиса AS `pairs()` пока нет.

### Стековые расширения C89/C++98 (этап 24)

Lua-подобный интерфейс описан [по-русски](doc/STACK_API.ru.md) и [по-английски](doc/STACK_API.md). Рабочие примеры находятся в `examples/extending/stack_c/` и `examples/embedding/cpp98_stack/`. Это не ABI Lua; непосредственный вызов стековых функций из AVM1 ещё не реализован.

### Вызываемые объекты C (этап 25)

См. [описание API](doc/CALLABLE_OBJECTS.ru.md) и
[пример C89](examples/extending/callable_objects/README.ru.md).
Нативные C-функции можно сохранять как значения объектов и вызывать через C API.
Замыкания AVM1 и `__call` пока не реализованы.

## Нативный `__call` (этап 26)

`include/flashvm/meta_call.h` предоставляет [`fvm_meta_call`](include/flashvm/meta_call.h#L20): обработчик получает исходный объект, метатаблицу и отдельную индексированную таблицу аргументов. Рабочий тест — `tests/meta_call.c`. Опциональные объекты потоков и каналов ActionScript реализованы через [`fvm_extensions_install`](include/flashvm/extensions.h#L17); проектный контракт описан в `doc/THREADS_CHANNELS.ru.md`.

### Внутренние каналы (этап 27)

См. [описание API каналов](doc/CHANNELS.ru.md) и [пример C89](examples/embedding/channels/README.md). Добавлена переносимая синхронизированная FIFO-очередь для C/C++. Потоки VM и методы каналов ActionScript доступны через [`fvm_extensions_install`](include/flashvm/extensions.h#L17); нестандартные операторы языка пока отсутствуют.

### Нативные рабочие потоки

См. [API потоков](doc/WORKERS.ru.md) и [пример C89](examples/embedding/workers/). Потоки создают независимые контексты интерпретатора через переходники `ports/`. ActionScript `thread.run` доступен после [`fvm_extensions_install`](include/flashvm/extensions.h#L17); см. `doc/ACTIONSCRIPT_EXTENSIONS.ru.md`.

Управление жизненным циклом потоков (отложенный запуск, состояния, ожидание):
[doc/WORKER_LIFECYCLE.ru.md](doc/WORKER_LIFECYCLE.ru.md).

### Кооперативная остановка рабочих потоков

Переносимый C89 API поддерживает запрос остановки потока и ожидание массива
потоков. См. [документацию](doc/WORKER_CANCELLATION.ru.md) и
`examples/embedding/worker_stop/`. Объект ActionScript `thread` доступен через [`fvm_extensions_install`](include/flashvm/extensions.h#L17).

## Обмен сообщениями между потоками

Рабочий пример C89 находится в [`examples/embedding/worker_channels/`](examples/embedding/worker_channels/). В [`doc/WORKER_CHANNELS.ru.md`](doc/WORKER_CHANNELS.ru.md) описаны блокирующее и неблокирующее чтение, время жизни строк, порядок остановки и ограничения.

Ограниченные очереди каналов и блокировка при заполнении: см. [doc/CHANNEL_CAPACITY.ru.md](doc/CHANNEL_CAPACITY.ru.md).

Совместное владение каналами описано в [doc/CHANNEL_OWNERSHIP.md](doc/CHANNEL_OWNERSHIP.md).

### Каналы с управляемым владением (этапы 34–38)

См. [русское описание API](doc/CHANNEL_VALUES.ru.md),
[английское описание](doc/CHANNEL_VALUES.md), примеры C89/C++98 в
`examples/embedding/channel_values`, `channel_cpp98` и
`examples/extending/channel_worker`. Пока это нативные дескрипторы, **не**
управляемые GC значения ActionScript.

## Сборщик мусора (экспериментальный)

`flashvm/gc.h` предоставляет сборщик mark-and-sweep для каждого контекста,
удержание объектов из C, настройку частоты и однократную финализацию.
Недостижимые **циклические ссылки** освобождаются. Корни стека AVM1 ещё
не интегрированы, поэтому сборку следует запускать только в явно безопасных
точках. См. [`doc/GARBAGE_COLLECTION.ru.md`](doc/GARBAGE_COLLECTION.ru.md)
и [`examples/embedding/gc/`](examples/embedding/gc/).
Объект ActionScript `gc` с методами `mode` и `state` пока только проектируется.

## Документация GC (этапы 49–68)

См. [безопасные точки](doc/GC_SAFE_POINTS.md), [режимы GC](doc/GC_MODES.ru.md), [указатель API](doc/API_INDEX.ru.md) и [диагностику](doc/GC_TROUBLESHOOTING.md). Сборку пока нельзя безопасно запускать внутри активных кадров AVM1.

Корни GC активных кадров: [English](doc/GC_ACTIVE_FRAMES.md) / [Русский](doc/GC_ACTIVE_FRAMES.ru.md).

### Временные корни GC

См. [временные области корней](doc/GC_NATIVE_ROOTS.ru.md): встраивание из C89, правила владения и ограничения.

### Безопасность GC и диагностика

См. [Безопасность GC при нативных вызовах](doc/GC_CALLBACK_SAFETY.ru.md):
защита от повторного запуска, корни и диагностика через C89 API.

### Автоматическая сборка мусора в AVM1

Интерпретатор проверяет GC между инструкциями при зарегистрированных корнях
активных кадров. Подробнее: [безопасные точки GC](doc/GC_AVM1_SAFE_POINTS.ru.md).

## Проверенное продолжение разработки

[Расширения ActionScript](doc/ACTIONSCRIPT_EXTENSIONS.ru.md), [контракты GC](doc/GC_COMPLETION.ru.md), [владение потоками и каналами](doc/CONCURRENCY_SAFETY.ru.md), [результаты проверок](doc/VALIDATION.ru.md). Добавлены ограниченные проходы финализации, release userdata, корни захваченных нативными callback объектов, безопасный одновременный join, независимое владение строками сообщений, снимки циклических графов и опциональные объекты `gc`/`thread`/`channel`.

Сборка по умолчанию не требует внешних системных библиотек. SQLite встроен исходниками и включён по умолчанию через DEFS.txt; исходники встроенного regex находятся в `external/`. На Linux без CMake: `python3 scripts/check.py`; ASan/UBSan: добавить `--sanitize`. Ограничения LeakSanitizer и платформ указаны в отчёте.

Файловые каналы и метатаблицы значений: [по-русски](doc/FILE_CHANNELS.ru.md), [по-английски](doc/FILE_CHANNELS.md).
Текущий ABI — 2: нативные клиенты необходимо пересобрать. [Отчёт и ограничения](doc/FILE_CHANNELS_CHECKPOINT.ru.md).

AVM1 decomposition / Декомпозиция: value-based C services and prepared native script
bodies for future JIT reuse; file map, GC contract, tests and limitations:
[AVM1_DECOMPOSITION.md](doc/AVM1_DECOMPOSITION.md).

Raw prototype primitive retention / Raw-значения прототипа: [RU/EN](doc/OBJECT_RAW_PROTOTYPE.md).

Shared execution budget/GC/stop checkpoints / Общие контрольные точки: [RU/EN](doc/EXECUTION_CHECKPOINT.md).

Loaded numeric text and strict NaN / Числовой текст и строгий NaN: [RU/EN](doc/AVM1_NUMBER_TEXT.md).

Named uppercase constants / Смысловые макросы: [header map and evidence](doc/SEMANTIC_CONSTANTS.md).

В v0.348 добавлен явный экспериментальный JIT Linux x86-64.
[Сборка и запуск примера интерпретатор/JIT](doc/JIT.md); полный JIT остаётся открытым.

Режимы интерпретации/JIT при вызове, переключение во время исполнения и
диагностический повтор после исключения: [API и пример](doc/EXECUTION_MODES.md).

Multi-architecture JIT, breakpoints, threads and multiple VM examples / JIT для разных CPU, отладка, потоки и несколько VM: [JIT_ARCHITECTURES.md](doc/JIT_ARCHITECTURES.md).

Подробное руководство: [JIT, отладка, потоки и несколько VM](doc/JIT_ARCHITECTURES.ru.md).

Нативные DefineFunction/DefineFunction2: [English](doc/JIT_DEFINITIONS.md) /
[Русский](doc/JIT_DEFINITIONS.ru.md); захват области/пула, callbacks и отказы памяти.

Нативный With: общий стек/регистры/пул, вложенный Return и восстановление области;
[English](doc/JIT_WITH.md) / [Русский](doc/JIT_WITH.ru.md).

Нативные Try/Catch/Finally: сохранение исключений/Return, catch по имени/регистру
и защита root-входа от OOM; [English](doc/JIT_TRY.md) / [Русский](doc/JIT_TRY.ru.md).

Borrowed negative splice / отрицательная длина: [EN](doc/ARRAY_NEGATIVE_SPLICE.md) / [RU](doc/ARRAY_NEGATIVE_SPLICE.ru.md).

Signed borrowed end operations / знаковые заимствованные методы: [EN](doc/ARRAY_NEGATIVE_ENDS.md) / [RU](doc/ARRAY_NEGATIVE_ENDS.ru.md).

Negative loaded slice / slice с отрицательной длиной: [EN](doc/ARRAY_NEGATIVE_SLICE.md) / [RU](doc/ARRAY_NEGATIVE_SLICE.ru.md).

Large loaded Array mutations / большие загруженные массивы: [EN](doc/ARRAY_LOADED_LARGE_MUTATION.md) / [RU](doc/ARRAY_LOADED_LARGE_MUTATION.ru.md).

Large loaded sorting / сортировка больших массивов: [EN](doc/ARRAY_LOADED_LARGE_SORT.md) / [RU](doc/ARRAY_LOADED_LARGE_SORT.ru.md).

Unicode legacy display paths / Unicode-пути старых SWF: [EN](doc/MOVIECLIP_UNICODE_PATHS.md) / [RU](doc/MOVIECLIP_UNICODE_PATHS.ru.md).

[Интерфейсы оригинальных функций](doc/REMOTE_FUNCTIONS.ru.md) — v0.474.

[Примеры RPC функций: процессы, независимые VM-потоки и LAN](doc/FUNCTION_RPC_EXAMPLES.ru.md).

[Автоматическое обновление версии в CMake](doc/BUILD_VERSION.ru.md).

[Загруженный Object.__resolve](doc/OBJECT_RESOLVE.ru.md) — v0.474: обработчик
отсутствующего свойства, counted-имена, регистр SWF, корни GC и предел рекурсии.

[Пример resolver на C89](examples/embedding/property_resolve/README.ru.md).

[Loaded builtin construction / Конструирование и вызовы SWF](doc/BUILTIN_CONSTRUCTION.md) — v0.474.

[Current-global primitive boxing / Текущие классы примитивов](doc/PRIMITIVE_GLOBAL_BOXING.md) — v0.474.

Debugger tutorials / Примеры отладчика: [examples/debugger](examples/debugger/README.md).
Complete API reference: [English](doc/DEBUG_API.md) / [Русский](doc/DEBUG_API.ru.md);
[SWF/SWD source mapping](doc/SWD_DEBUGGING.md).

Полный каталог примеров / Complete example catalog: [examples/README.md](examples/README.md).
Documentation source links and maintenance / Правила документации: [RU/EN](doc/DOCUMENTATION_GUIDELINES.md).
Stage quality / Качество сцены: [API and ToggleQuality](doc/STAGE_QUALITY.md).
[Multimedia and device bridges / Мосты мультимедиа и устройств](doc/MULTIMEDIA_BRIDGES.md): static FFmpeg and replaceable host callbacks.
[Native speech / Системные TTS и STT](doc/SPEECH_BRIDGES.md): explicit providers and per-OS prepared-host examples.
## Лицензия

Собственный код и документация FlashVM распространяются по
[лицензии MIT](../LICENSE). Copyright (c) 2026 bhgv.
Сторонний код, данные и зависимости сохраняют свои авторские уведомления и
лицензии: корневая MIT их не заменяет. См. уведомления в
[external/](external/README.md), каталогах соответствующих зависимостей и
отмеченных сторонних фрагментах исходников проекта.
## Локальный просмотр документации

Установите `python -m pip install -r doku_viewer/requirements.txt`, затем
запустите `python view_docs.py`. Python 2.7.9/3, навигация по Markdown и
подсвеченные исходники с переходом к строке: [руководство](doku_viewer/README.md).
## Десктопное приложение

[Frontend](frontend/README.ru.md): необязательный SDL2-проигрыватель для
Linux/Windows/macOS, SWF-пути, JIT/interpreter, качество, FPS/проценты и справка.
[Полное руководство](doc/DESKTOP_FRONTEND.ru.md).
Порционный StartSound-микшер: [руководство](doc/SWF_SOUND_MIXER.ru.md),
[учебник](examples/multimedia/swf_event_mixer/README.md); перекрытие голосов,
stop/no-multiple, циклы и огибающие без новых выделений при чтении.
Root timeline sound delivery: [EN](doc/MOVIECLIP_SOUND.md) /
[RU](doc/MOVIECLIP_SOUND.ru.md); actual frame entry before DoAction,
source checks and host-local callback registration.
Explicit mixed-rate/channel audio: [EN](doc/SWF_SOUND_CONVERSION.md) /
[RU](doc/SWF_SOUND_CONVERSION.ru.md); configured output including 48000 Hz stereo,
with legacy strict API preserved and an annotated --convert-demo tutorial.
v0.583: отдельный audio worker и [StopSounds](doc/AUDIO_STOP_BINDINGS.ru.md); исправление треска Android требует проверки на телефоне.
[Проверяемые результаты платформенной приёмки](doc/RELEASE_EVIDENCE.ru.md).
[Сборка для своей и другой ОС/CPU](../BUILDING.ru.md).
Binary S-record profile / Двоичные S-записи: [EN](doc/SRECORD.md) / [RU](doc/SRECORD.ru.md).
Resource namespace design / План пространства ресурсов: [EN](doc/RESOURCE_NAMESPACE.md) / [RU](doc/RESOURCE_NAMESPACE.ru.md).
Resource owner control / Управление ресурсами у владельца: [EN](doc/RESOURCE_DISPATCH.md) / [RU](doc/RESOURCE_DISPATCH.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](include/flashvm/flashvm.h#L74) |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:17](include/flashvm/extensions.h#L17) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](include/flashvm/meta_call.h#L20) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:92](include/flashvm/swf_stream_output.h#L92) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
