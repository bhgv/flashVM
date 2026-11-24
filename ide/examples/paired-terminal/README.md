# Compiled DisVM and FlashVM terminal

[Русский](README.ru.md)

The two bytecode files exercise successive view updates on the paired IDE transport.
The Limbo application uploads the linked movie to `.upload`, attaches two
ordinary `chan of array of byte` channels with `$Unibus.attach` and starts playback
by writing `start` to `.state`. It then sends two fvd1 views with Unicode payloads.
The ActionScript `onEnterFrame` handler reads from `styxTerminal`, applies the requested
horizontal offsets of 64 and 96 pixels, and returns each payload with the suffix
` AS2`. It retains an unaccepted reply until the terminal has credit. Limbo checks
both replies in order and detaches. Both paths use the same libstyxrpc session.

## Run

Run the included `terminal.dis` and `terminal.swf` with the project's mounted
`disvm` executable and the SDK-enabled
`flashvm_styx_player`. On POSIX, the optional Python 3 launcher creates the
socketpair, passes file descriptors to the two executables and checks exit
status. Application logic runs entirely from `terminal.dis` and `terminal.swf`.
Delivery takes place while the movie is running. Its callback polls once per tick.

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
Success prints `PASS compiled AS2 + Limbo` with the two echoed frames.

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
The linker imports the rectangle from `panel.svg` with `--svg`. The Limbo frontend checks
`terminal.b` and the project's bootstrap compiler resolves module types and
emits `terminal.dis`. Only the two bytecode files are needed by the guest
applications.
