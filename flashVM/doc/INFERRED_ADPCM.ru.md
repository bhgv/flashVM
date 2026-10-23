# Подготовка коротких ADPCM-блоков по геометрии байтов


v0.1064: Добавлена bounded-подготовка с прежними whole packets и единственной partial-длиной без header average.

v0.1065: Для 104 геометрий независимо перебираются длины и проверяется каждый PCM sample, неоднозначность и legacy whole packets.

v0.1066: Проверены четыре native-аллокации, точный и недостаточный бюджет, уничтожение VM и чтение PCM без аллокаций.

v0.1067: Проверены неверные окончания, короткий predictor, неоднозначный mono и единственное nonzero-окончание с точным PCM.

v0.1068: Desktop-сегменты используют inferred bounded snapshot с общим peak-бюджетом и ошибкой до открытия устройства.

v0.1069: Проверены FWS/CWS, interpreter/JIT, точный PCM, restart, пустые блоки, PCM successor и отказ до устройства.

v0.1070: Worker проверяет single-saturation stereo PCM и BASE19/TAIL0 для обоих finite-режимов, CWS/FWS и двух VM modes.
