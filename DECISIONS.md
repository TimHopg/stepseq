# Decisions Log

Succinct record of notable technical decisions and why they were made. Newest at the bottom.

## 2026-08-28 — Language & standard: C++20

Chosen to showcase modern C++ (concepts, ranges, `std::span`, coroutines where useful) for
a CV project targeting C++ developer roles.

## 2026-08-28 — Domain: step sequencer / music app

Leverages prior music industry experience; naturally exercises timing, concurrency, binary
I/O, and event modeling without feeling like a generic tutorial project.

## 2026-08-28 — Built-in synth instead of MIDI-only output

MIDI-only would need an external DAW/softsynth to hear anything, hurting "clone and run"
demoability. A small internal synth (oscillators + envelope + mixer) played via miniaudio
keeps the project self-contained.

## 2026-08-28 — Audio library: miniaudio over RtAudio

Single-header, minimal build-system friction across platforms. RtAudio is lower-level and
arguably more "serious," but fussier to build reliably for a demo-focused project. miniaudio
only handles device I/O — synthesis is hand-written.

## 2026-08-28 — Input model v1: REPL with tracker-style pattern strings, not a live TUI grid

A per-note REPL (`set step 3 note C4`) was rejected as tedious. A live terminal grid
(FTXUI) was considered and is planned for v1.1, but deferred to keep v1 small. Tracker-style
strings (`kick x..x..x..x..`) give fast, idiomatic input with no extra dependency or
raw-terminal handling.

## 2026-08-28 — MIDI export before import

Writing a MIDI file is a contained binary-format exercise. Robust parsing (running status,
variable-length quantities, malformed files) is a bigger job on its own. Export ships in v1;
import is deferred to v1.1.

## 2026-08-28 — Save/load format: JSON via nlohmann/json, not hand-rolled

JSON serialization isn't a skill this project needs to prove. Using an established library
here (vs. hand-rolling, as we're doing for MIDI) demonstrates judgment about when to reach
for a library versus build it yourself.

## 2026-08-28 — Testing framework: Catch2 over GoogleTest

Lighter to wire into CMake via FetchContent; BDD-style syntax reads well in a portfolio repo.

## 2026-08-28 — JUCE rejected for v1/v1.1

JUCE would do the interesting work for us (GUI, audio device abstraction, DSP utilities),
undermining the point of hand-rolling the synth/engine. It also adds build-tooling and
licensing overhead not worth taking on now. Possible future consideration only if a native
GUI is built specifically to target audio-software employers — not part of this plan.

## 2026-08-28 — `Step` type starts as just `{ bool active }`

Three of the four v1 voices (kick/snare/hat) are pure on/off triggers; only the synth voice
needs a note. Adding note support now would mean designing for a requirement we haven't
reached yet. `Step` will grow a note representation when the synth voice actually needs it.

## 2026-08-28 — Plain public-field structs for types with no invariants

`Step` (and future simple data types) use a public-field `struct` rather than a `class` with
getters/setters. Encapsulation earns its keep when there's an invariant to protect
(validation, keeping fields consistent); a bare `bool` has none, so accessors would only add
indirection with no safety benefit. Revisit per-type if/when a real invariant appears.

## 2026-08-28 — Headers under `include/stepseq/`, include dirs `PRIVATE` for now

Each type gets its own header under `include/stepseq/`, included as `<stepseq/name.hpp>`.
`target_include_directories` is `PRIVATE` on `stepseq_tests` since nothing yet links against
it; revisit as `PUBLIC`/`INTERFACE` once a shared library target exists for `stepseq` and
`stepseq_tests` to both depend on.
