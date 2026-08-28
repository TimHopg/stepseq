# Step Sequencer

A command-line step sequencer with a built-in synth engine, written in modern C++20.

Type a pattern in, hear it play back live — no DAW or external synth required.

```
kick  x..x..x..x..x..x.
snare ....x.......x...
hat   x.x.x.x.x.x.x.x.
```

## Status

Early development. See [DECISIONS.md](./DECISIONS.md) for the reasoning behind key
technical choices, and [CLAUDE.md](./CLAUDE.md) for how this project is being built.

## Building

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
./build/stepseq
```
