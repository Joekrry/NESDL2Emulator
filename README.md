# nesemu

An NES emulator written in C11, built to boot a homebrew Tetris ROM (NROM
mapper), render it via SDL2, and accept keyboard/gamepad input.

Interpreter-only (no dynarec), with cycle-accurate CPU/PPU scheduling, a
software PPU (background + sprites), and a full APU (2 pulse, triangle,
noise, DMC).

## Building

Requires CMake >= 3.16 and SDL2 development headers.

```sh
cmake -B build
cmake --build build
./build/nesemu <path-to-rom>
```

Debug build with sanitizers:

```sh
cmake -B build -DNES_ASAN=ON
cmake --build build
```

Press Escape or close the window to exit.
