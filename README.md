# Step Sequencer

A command-line step sequencer with a built-in synth engine, written in modern C++20.

Type a pattern in, hear it play back live — no DAW or external synth required.

```plaintext
kick  x..x..x..x..x..x.
snare ....x.......x...
hat   x.x.x.x.x.x.x.x.
```

## Status

Early development. See [DECISIONS.md](./DECISIONS.md) for the reasoning behind key
technical choices, and [CLAUDE.md](./CLAUDE.md) for how this project is being built.

## Building

Requires CMake 3.20+ and a C++20 compiler. Catch2 is fetched by CMake on first configure,
so that step needs network access. Ninja is used if it is installed, because its no-op build
is quicker; without it CMake's default generator is used instead.

```sh
make          # configure (first time) + build
make test     # build, then run the test suite
make run      # build, then start the REPL
make clean    # remove the build directory
```

The `Makefile` is only a thin wrapper — `cmake -S . -B build && cmake --build build` works
just as well.
