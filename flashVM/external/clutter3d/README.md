# Selected Clutter3D library source / Выбранная библиотека Clutter3D

Source: **https://github.com/bhgv/Inferno-OS_Android-Clutter3d**.
Pinned revision: **2a616b264f0045a3ba00ce77aa1930ca905956ea**.
This directory contains source selection/build instructions and a minimal patch,
not another unrelated Clutter implementation. The upstream library remains an
independent SDK. `inferno-os` is excluded from the exported SDK source tree,
compilation and linking. No libinterp/libdraw/client symbols are used by FlashVM.

## Prepare the actual fork

Needs Git, patch, and Python ≥3.12 for safe tar extraction. The explicit local
Git repository can be a no-checkout partial clone; the exporter reads only the
pinned library/glue paths and root license files. It refuses a nonempty output.

```sh
git clone --filter=blob:none --no-checkout https://github.com/bhgv/Inferno-OS_Android-Clutter3d.git /absolute/clutter3d-git
python3 external/clutter3d/prepare_sources.py --repository /absolute/clutter3d-git --output /absolute/clutter3d-src
```

Exported: sources/clutter-android, sources/cogl-android,
sources/glib-android, fake-android-application and root COPYING/LICENSE.
Nothing under inferno-os is extracted. The SDK manifest records repository,
revision, exported/excluded paths and applied patch SHA-256. Prepared sources
remain LGPL/upstream licensed as stated by their own COPYING; preparation does
not relabel third-party code as FlashVM MIT or modify the source Git repository.

Русский: выполняется экспорт именно библиотечных исходников вашего Git,
а не подмена системной версией. Клиент исключён физически из дерева SDK.
Выходной каталог должен быть новым/пустым. Manifest хранит revision и hash
патча; Git-оригинал не переписывается. Лицензии сохраняются.

## Linux static library build

Provide a native C/C++ toolchain, make, pkg-config, autoconf, automake ≥1.11,
libtool, gettext/autopoint, gtk-doc-tools, autotools-dev and SDKs for GLib,
GObject/GIO, Pango/PangoFT2, Cairo, ATK, JSON-GLib, GdkPixbuf, Fontconfig/FreeType,
X11/Xext/Xfixes/Xdamage/Xcomposite/Xrandr/Xi and OpenGL/GLX. On Ubuntu/Debian,
packages include build-essential, autoconf, automake, libtool, gettext,
autopoint, gtk-doc-tools, autotools-dev, pkg-config, libglib2.0-dev,
libpango1.0-dev, libcairo2-dev, libatk1.0-dev, libjson-glib-dev,
libgdk-pixbuf-2.0-dev, libgl-dev, libx11-dev, libxext-dev, libxfixes-dev,
libxdamage-dev, libxcomposite-dev, libxrandr-dev and libxi-dev. Use your
distribution's matching versions; no dependency is fetched implicitly by CMake.
For a private SDK export its pkg-config/include/library paths explicitly.

```sh
sh external/clutter3d/build_linux.sh /absolute/clutter3d-src /absolute/clutter3d-sdk 4
cmake -S . -B build-clutter -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_CLUTTER3D=ON -DFVM_CLUTTER3D_SDK=/absolute/clutter3d-sdk
cmake --build build-clutter --target flashvm_example_clutter3d
./build-clutter/examples/frontends/flashvm_example_clutter3d --smoke
```

The script builds **both fork Cogl and fork Clutter statically**, preferring
matching installed Cogl headers/pkg-config over system versions. Other SDK
libraries may remain shared in this validation profile. This is a Linux
X11/OpenGL/GLX profile, not a certified static-all-dependencies release or
GLES/Android build. Failure aborts immediately; only a successful pair gets the
SDK provenance marker accepted by the example's default CMake route.

For an explicitly auxiliary system-SDK compatibility test only, set
FVM_CLUTTER3D_SYSTEM_TEST_SDK=ON without FVM_CLUTTER3D_SDK. It is OFF by default.
Do not present that run as the selected fork or silently mix its SDK files.
Use a fresh CMake build directory when switching providers.

## Compatibility fixes / Исправления

[linux-android-config.patch](patches/linux-android-config.patch) changes only
upstream configure.ac for this Linux/Android SDK profile:

1. Initialize AC_PROG_CXX before LT_INIT on **all** platforms. Optional Android
   C++ sources make Automake choose CXXLINK even for Linux. Previously C++
   libtool initialization ran only in the Android branch; on Linux the CXX
   archive command could be empty, returning success without producing a library.
2. Stop unconditionally generating clutter/config.h.win32 from its **missing**
   input template. It is unused by Linux/Android, yet blocked modern autoreconf.
   This narrowed SDK profile does not supply or claim a Windows Clutter build;
   a future Windows SDK must restore its actual template explicitly.

The existing Android AC_PROG_CXX invocation is harmless after unconditional
initialization. Upstream source/API/ownership rules and copyright notices are
preserved. Modern gettext/automake regeneration changes are build output and
are not included in this patch.

Русский: исправлены безусловная настройка C++-тега libtool и ненужная генерация
отсутствующего Win32-шаблона. Профиль предназначен для Linux/Android; это не
исправление Windows-порта. Исходники и зависимости библиотеки принадлежат
отдельному SDK, не соседнему inferno-os.

## Android / GLES

The export retains the fork's clutter/android backend, its native glue,
Cogl Android winsys and glib-android sources. An Android SDK must cross-build
the required GLib-Android/dependencies for the exact NDK ABI/API, configure Cogl
for EGL/GLES and Clutter with --enable-android-backend (disable desktop X11),
and link the application's actual native_app_glue/Android/log/EGL/GLES
libraries. The Linux script deliberately does not invent an Android toolchain
or reuse desktop libraries in an APK. Follow the fork's Android build options
and integrate with the shell-owned application ready/surface lifecycle as in
[the tutorial](../../examples/frontends/clutter3d/README.md#android-shell-integration--подключение-к-android-оболочке).
No client library from inferno-os is part of that integration. Native Android
build/run, context-loss recovery and phone/GLES acceptance remain open.

## Executed validation (v0.600)

A clean pinned export, including the patch above, built both Cogl 1.14.1 and
Clutter 1.14.1 static archives with GCC 13 on Linux x64. The C89 example linked
those archives, without a dynamic libclutter/libcogl dependency. Under Xvfb/Mesa
software GLX, its real SDK fixture checked RGBA texture readback, first-contact
selection/cancellation, native deactivation and projected actor coordinates;
normal and touch-only smoke runs each completed 120 uploads. The host/VM fixture
also passed ASan/UBSan (SDK uninstrumented, leak detection disabled). The system
Clutter 1.26 compatibility run was separate auxiliary evidence. No actual Android,
EGL/GLES backend, physical GPU or full static-dependency release is claimed.
