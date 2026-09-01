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

## 2026-08-31 — `Track` step count: fixed-size `std::array<Step, 16>`, not a runtime length

v1 only needs a single fixed 16-step pattern, so the length is a compile-time constant
(`kStepsPerTrack`) rather than a constructor parameter or `std::vector`. This also means
`Track` has no length invariant to protect, so — per the `Step` plain-struct decision — it
stays a public-field struct too. Revisit if a future version needs variable pattern lengths.

## 2026-09-01 — `Pattern::Pattern` takes its `tracks` array by value + `std::move`s it, not `const&`

A "sink parameter" — the constructor consumes the argument, so it takes ownership by value
rather than borrowing it. Cost: at most one copy (when the caller passes an lvalue), same as
`const&` would cost. Benefit: when the caller passes a temporary/rvalue, the parameter itself
is move-constructed (cheap) and then moved again into the member (cheap) — no copy at all.
So by-value-then-move is never worse than `const&`-then-copy, and is free in the common case
of constructing a `Pattern` from a freshly-built array, without needing separate `const&`/`&&`
overloads.

## 2026-09-01 — `Pattern` is a class; `bpm` is validated and throws, rather than being clamped

`Pattern` is the first type with a real invariant (`bpm > 0`), so — unlike `Step`/`Track` — it's
a `class`: `bpm_` is private and validated in the constructor (throwing `std::invalid_argument`
on `bpm <= 0`), while `tracks` stays a public field since it has no invariant to protect.

Considered clamping bad `bpm` to a valid range instead of throwing, since a live tool shouldn't
crash a session over a typo. Rejected for the type itself: `Pattern` will eventually be
constructed not just from live REPL input but from deserialized JSON save files, where a
negative/zero `bpm` means a corrupted or malicious file, not a forgivable typo — clamping there
would silently load bad data instead of surfacing the problem. Split responsibility instead:
`Pattern`'s constructor stays strict (throw = "this should never happen if the caller behaved"),
and forgiving behavior (clamping a bad tempo typed by a human) belongs in the REPL layer, which
validates/clamps *before* ever constructing a `Pattern`. Defense in depth: forgiving UI, strict
type.

## 2026-09-01 — `parseSteps` stays a free function in its own header, depending on `Track`'s constant

`include/stepseq/steps_parser.hpp` reaches into `track.hpp` solely to reuse `kStepsPerTrack`,
even though nothing else in the file needs `Track`. Considered making it a named factory
instead (`Track::fromPattern(name, pattern)`), which would keep the constant's only consumer
under `Track` itself. Left as a free function for now — no REPL exists yet to show which shape
reads better in practice. Revisit once the REPL is wiring tracker strings into `Track`s.
