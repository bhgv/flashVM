# Android boundary / Граница Android

Android is not a desktop argv/window environment. This directory intentionally
contains no fake main returning success: the desktop target is rejected when
cross-configuring for Android. An Android Activity/JNI/SDLActivity package,
intent-to-file handling and lifecycle/surface integration still need a separate
implementation and native device validation. Existing VM Android ports remain
available independently with the desktop build option disabled.

Android не является окружением десктопного CLI. Здесь нет фиктивного main:
сборка десктопной цели для Android явно отклоняется. Activity/JNI/SDLActivity,
открытие файлов через intent, жизненный цикл поверхности и реальный прогон на
устройстве ещё нужны. Существующие Android-порты VM доступны при выключенной
опции десктопного frontend. Это незакрытая часть стадии 9.
