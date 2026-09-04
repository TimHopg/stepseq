# Decisions Log

Succinct record of notable technical decisions and why they were made. Newest at the bottom.

## 2026-08-28 — Language & standard: C++20

Chosen to showcase modern C++ (concepts, ranges, `std::span`, coroutines where useful) for
a CV project targeting C++ developer roles.

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

## 2026-08-28 — Save/load format: JSON via nlohmann/json, not our own format

JSON serialization isn't a skill this project needs to prove. Using an established library
here (vs. hand-rolling, as we're doing for MIDI) demonstrates judgment about when to reach
for a library versus build it yourself.

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

## 2026-09-04 — `runRepl` takes `istream&`/`ostream&`; errors print and the loop continues

`runRepl(std::istream&, std::ostream&, Pattern&)` takes the abstract stream bases rather than
using `std::cin`/`std::cout` directly, so tests bind `istringstream`/`ostringstream` and `main`
binds the real streams. That seam is the reason the command loop is unit-testable at all.

Contract: nothing propagates out of `runRepl`. Unrecognized input prints a one-line
`error: ...` and the loop carries on — the "forgiving UI, strict type" split already recorded
for `Pattern`. The upcoming `bpm <n>` command will catch `Pattern`'s `std::invalid_argument`
and report it this way rather than letting it escape, and will constrain input to a sane
musical range — which also keeps `print` from ever rendering a tempo in scientific notation,
so `printPattern` needs no stream-formatting code of its own.

A prompt is written before every read, with a newline emitted on end-of-input to close the
dangling prompt line. Added now rather than later because every exact-output test would
otherwise need rewriting to accommodate it.

The trailing `'\r'` from CRLF input is stripped once, centrally, right after `getline`, rather
than leaving each command to tolerate it. It currently *appears* to work without this because
`'\r'` is whitespace and `operator>>` skips it — but the next slice reads the remainder of the
line, which would hand `parseSteps` 17 characters on Windows line endings.

## 2026-09-04 — Pattern rendering lives in `repl.hpp`, not as `Pattern`'s `operator<<`

`printPattern(std::ostream&, const Pattern&)` sits alongside `runRepl`. Considered making it
`operator<<` on `Pattern` instead, which would put rendering with the type it renders and would
make Catch2 print the pattern instead of `{?}` on a failed assertion.

Kept in the REPL layer because this is specifically the *REPL's* text format: JSON save files,
MIDI export, and the v1.1 terminal grid will each render a `Pattern` differently, and none of
them should inherit this one. Stream-first argument order matches `operator<<` and the
sink-first convention, so it can become one later without churn if a second consumer wants it.

## 2026-09-04 — The v1 voice set and banner live in `repl.hpp`; `main` is thin wiring

`makeDefaultPattern()` and `kDefaultBpm` sit in `repl.hpp`, not `pattern.hpp`. Which four
voices v1 ships with is application policy; `Pattern` is a container with one invariant
(`bpm > 0`) and knows nothing about voice names. Putting them in the type header would have
meant every consumer of the type also saw the policy, and `pattern_test.cpp` would have become
the place that pins product decisions rather than type behaviour. Considered a dedicated
`app_defaults.hpp` for cleaner layering, rejected as a whole header for one function and one
constant. They stay in a header rather than moving into `main()` purely so tests can reach them.

The startup banner is likewise printed inside `runRepl` through the injected `ostream&`, not
via `std::cout` from `main`. It previously bypassed the very seam that makes the loop testable,
so nothing could catch it drifting out of step with the command set it advertises. Through
`out` it falls under the same exact-output tests as everything else.

The banner deliberately advertises `quit` only, treating it as the canonical spelling, even
though `exit` is accepted as a synonym — listing both reads as clutter in a one-line greeting.
It is also not an exhaustive command list and isn't meant to become one: once there are more
than a handful of commands, discoverability belongs in a `help` command that can be tested
against the dispatch, not in a greeting that has to be kept in sync by hand.

`main` therefore does nothing but construct the default pattern and hand it to `runRepl`, and
returns 0 unconditionally with no top-level `try`/`catch`. Nothing reachable from it can throw
in practice: `kDefaultBpm` is a positive constant so `validateBpm`'s throw is unreachable, and
`runRepl`'s contract is that nothing propagates. The moment to add a top-level catch returning
non-zero is when JSON load lands — the first path that can legitimately throw from outside the
command loop.

## 2026-09-04 — Adopted Google C++ Style Guide for naming/formatting only

Most of the naming already matched by convention (`PascalCase` types, `kCamelCase` constants,
trailing-underscore private members). Formalized that plus include ordering and pointer/reference
placement in CLAUDE.md.

Deliberately did not adopt the parts of Google's guide driven by managing a huge legacy
monorepo across thousands of engineers: it bans exceptions outright, which conflicts with the
throwing-validation design already chosen for `Pattern`/`parseSteps`. Also kept functions
`camelCase` rather than Google's `PascalCase` (avoids reformatting everything already written),
4-space indentation over Google's 2-space, and `#pragma once` over its `#ifndef` guard macros.

## 2026-09-04 — Fixed-width label column in `print`, padded by hand

Step grids only read as a grid if every row's steps start at the same column, so `printPattern`
pads `name:` out to a fixed `kLabelWidth` (8) and abridges any name that will not fit. A tab was
the obvious alternative and was rejected: a tab advances to the next tab stop, so the gap depends
on the name's length and on the terminal's tab width — `hat` and `snare` would land on the same
stop while an 8-character name jumps an extra one.

The width is fixed rather than derived from the longest name in the pattern. Deriving it would
never truncate, but the columns would then shift whenever a name changed, and two patterns
printed one after the other would not line up with each other.

The padding is built as a string of spaces (`std::string(n, ' ')`) rather than with
`out << std::left << std::setw(...)`.
`std::setw` is one-shot but `std::left` is a *sticky* stream flag, so the iomanip version would
silently left-align everything the caller printed to that stream afterwards — and the stream here
is `std::cout`, owned by `main`. The string is at most 7 characters, so it stays inside the SSO
buffer and allocates nothing.

Byte-wise truncation is a known limitation: it counts bytes, not glyphs, so a non-ASCII name
would misalign and could be cut mid-sequence. Unreachable while names are hardcoded ASCII;
revisit if track names ever become user-settable.

## 2026-09-04 — Ninja as the CMake generator

`make run` and `make test` always invoke `cmake --build` so they can never launch a stale
binary. That check costs the same whether or not anything changed, and with CMake's default
Unix Makefiles generator it was taking ~2s per invocation on this machine, where the repo
sits on `/mnt/c` and every file stat crosses WSL2's Windows-filesystem bridge. Ninja keeps a
real dependency graph instead of re-stating everything, which measured ~0.7s for the same
no-op — about 3x.

Ninja is now a build prerequisite, which is the cost of the change. The raw
`cmake -S . -B build` still works with whatever generator is available, so it is only the
`Makefile` shortcut that requires it.

Most of the remaining 0.7s is the `/mnt/c` bridge, not the generator: the identical no-op
runs in ~0.02s with the build directory on the Linux filesystem. Moving the repo off `/mnt/c`
is the larger win still on the table.
