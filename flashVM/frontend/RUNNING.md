# Run the desktop player

From the repository root:

```sh
bash build-workspace.sh --projects vm --vm-profiles player --jobs 4
./out/flashvm/run-demo.sh
```

The first command builds and installs the actual SDL2 window/input/audio frontend.
A missing SDL2 development SDK is built automatically from pinned sources. The
first build needs network access; later builds reuse the local SDK cache. No
administrator installation or external Inferno emulator is required.

The default demonstration animates graphics and handles mouse/keyboard input and
sound. Its commented sources are installed alongside its SWF. Close the window
to exit, or bound a diagnostic run with `--frames 120 --stats`. Use `--mute` to
run without opening an audio device. `--no-jit` selects the interpreter.

`out/run-checks.sh` checks the installed numeric console example;
`out/run-checks.sh --window` starts the graphical example. Packages expose
`flashvm-console` separately: it only executes the supported action subset and
cannot display movies. In SDL2 packages the generic `flashvm` command is the real
player. This distinction fixes the old misleading headless alias and its -4
errors on graphical movies.

`--no-player` explicitly builds a console-only package. `--vm-profiles minimal`
also avoids graphics and SDL. Video fixtures are supplied as compatibility tests;
their presence is not a claim of supported video decoding.

Automated validation uses SDL dummy video/audio where physical devices are not
available. Such tests verify frame execution/presentation and queued sound, but
physical X11 windows, keyboard, mouse and speakers must also be tried on the target.
