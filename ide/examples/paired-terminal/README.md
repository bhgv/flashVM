# Compiled DisVM and FlashVM terminal

[Русский](README.ru.md)

The two bytecode files exercise one view update on the paired IDE transport.
The Limbo application uploads the linked movie to `.upload`, attaches two
ordinary `chan of array of byte` channels with `$Unibus.attach` and sends a
portable fvd1 view frame. It starts playback by writing `start` to `.state`.
The ActionScript receives the frame through `styxTerminal`, moves a panel
64 pixels to the right and returns the same Unicode payload with the suffix
` AS2`. Limbo compares the complete result and detaches. Both paths use the
same libstyxrpc session. This sample covers the initial view round trip.

## Run

Run the included `terminal.dis` and `terminal.swf` with the project's mounted
`disvm` executable and the SDK-enabled
`flashvm_styx_player`. On POSIX, the optional Python 3 launcher creates the
socketpair, passes file descriptors to the two executables and checks exit
status. Application logic runs entirely from `terminal.dis` and `terminal.swf`.
The sample uses DisVM's default controller scheduler so the unbuffered send is
pumped before the controller resumes to start the one-shot SWF.

```sh
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player --jit --capture frame.ppm
```

The launcher defaults to interpretation. `--jit` selects JIT in both VMs.
`--directory DIR` selects the directory containing both compiled files.
`--window` opens the SDL2 window for the short exchange. `--capture` also opens
the window and writes the checked final 320×240 image to the specified PPM
path. Use the graphics/SDL2-enabled player for either option. With
`SDL_VIDEODRIVER=dummy` the same raster check runs without a physical display.
Success prints `PASS compiled AS2 + Limbo`, followed by the echoed frame.

## Build

Use the project's [AS2 view compiler](https://github.com/bhgv/flashVM/blob/master/compiler/as2/view/README.md),
[FIR tools](https://github.com/bhgv/flashVM/blob/master/fir/v1/doc/PIPELINE.md),
[SWL](https://github.com/bhgv/flashVM/blob/master/linker/README.md),
[Coco/R Limbo frontend](https://github.com/bhgv/flashVM/blob/master/compiler/limbo/frontend/README.md) and the
in-tree `limbo-bootstrap` from the DisVM build. Pass their executable paths,
the directory containing `fir1` and `fir1_opt`, the DisVM source directory
containing the `Sys` and `Unibus` interfaces, and the build output directory:

```sh
sh build.sh /build/as2/view/as2_viewc /build/fir /build/linker/swl \
  /build/limbo/frontend/limbo_parse /build/disvm/bootstrap/limbo-bootstrap \
  /source/flashVM/disVM /output/terminal
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player --directory /output/terminal
```

`terminal.as` goes through AS2 → verified and optimized FIR → SWO → SWL → SWF.
The linker supplies the rectangle from `panel.svf`. The Limbo frontend checks
`terminal.b` and the project's bootstrap compiler resolves module types and
emits `terminal.dis`. Only the two bytecode files are needed by the guest
applications.
