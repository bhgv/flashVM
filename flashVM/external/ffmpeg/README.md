# Static FFmpeg SDK / Статический SDK FFmpeg

The MEDIA build uses **library calls**, never an ffmpeg subprocess. The optional
SDK is deliberately built in an explicit step; project configuration performs no
network access. Default NO_MEDIA builds need no FFmpeg and can still use custom
media/output/device callbacks. Dependency sources/build/install stay under this
folder and are ignored by Git; first-party adapters live in `src/multimedia/`.

```sh
python3 external/ffmpeg/build.py --prefix "$PWD/external/ffmpeg/install" --jobs 4
cmake -S . -B build-media -DFVM_DEFS_FILE=cmake/profiles/media.defs \
  -DFVM_FFMPEG_ROOT="$PWD/external/ffmpeg/install"
cmake --build build-media -j4
ctest --test-dir build-media --output-on-failure
```

The recipe requires Linux, Python 3.12+ (safe tar extraction), make, a C compiler,
and the usual Unix development headers. `--archive /path/ffmpeg-6.1.2.tar.xz`
uses an existing official archive after the same SHA256 verification.
`--work /path` selects disposable source/build storage. The script **replaces**
its own `ffmpeg-6.1.2` and `build` work subdirectories on each run; use a dedicated
work directory. It does not remove the installation prefix. The default work
folder is `external/ffmpeg/work`; default instructions use
`external/ffmpeg/install` as the explicit prefix.

Pinned source: <https://ffmpeg.org/releases/ffmpeg-6.1.2.tar.xz>.
SHA256: `3b624649725ecdc565c903ca6643d41f33bd49239922e45c9b1442c63dca4e38`.
No GPL or nonfree options are enabled. The source package contains FFmpeg's
license and notices; distributors must preserve applicable LGPL notices and
satisfy the upstream license requirements for their final distribution.

Enabled: static avdevice, avformat, avcodec, swresample, swscale and avutil;
file protocol; WAV/MP3/FLV/MOV/Matroska/Ogg/raw/image2/NUT demuxing; PCM S16LE/U8,
MP3, AAC, FLV1, H.263, VP6/VP6F, H.264, rawvideo, BMP, PPM, PNG and Vorbis decoding;
Linux V4L2 input. Programs, network protocols, automatic external-library
detection, x86 assembly, avfilter and postproc are disabled. This is a deliberately
bounded initial SDK, not every FFmpeg codec/device. FFmpeg can change its own
selected internal dependency list; inspect the generated configure summary.

For Windows, macOS or Android, supply a **target-matching static 6.1 SDK** with
`include/` and `lib/`, built using that SDK's supported toolchain. This Linux
recipe is not a cross compiler. CMake requires every static archive and never
falls back to shared libraries. `FVM_FFMPEG_SYSTEM_LIBRARIES` supplies additional
target system libraries/frameworks required by a custom SDK, for example capture
frameworks; do not claim another OS was tested merely because paths configure.
FFmpeg headers are private system includes; the public ABI contains no FFmpeg
objects. See [multimedia bridge guide](../../doc/MULTIMEDIA_BRIDGES.md).

RU: изначальный backend действительно вызывает FFmpeg. Загрузка исходников и
статическая сборка выполняются отдельной явной командой с проверкой SHA256.
Настройка FlashVM ничего не скачивает. На Linux рецепт проверен; для остальных
ОС нужен соответствующий SDK и самостоятельная платформенная проверка. Само
декодирование не включает воспроизведение, окно, разрешения камеры или микрофона.
