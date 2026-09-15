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

Ninja is detected rather than required: the `Makefile` passes `-G Ninja` only when
`command -v ninja` finds it, and otherwise lets CMake pick its default. Making it a hard
prerequisite was the first attempt and was wrong — it broke the wrapper on any machine
without Ninja, which is the one thing a convenience wrapper must not do.

Most of the remaining 0.7s is the `/mnt/c` bridge, not the generator: the identical no-op
runs in ~0.02s with the build directory on the Linux filesystem. Moving the repo off `/mnt/c`
is the larger win still on the table.

## 2026-09-11 — `findTrack` is a `Pattern` member, not a REPL free function

Name-based track lookup started as a free `findTrack(Pattern&, std::string_view)` in
`repl.hpp` and moved onto `Pattern`. It reads nothing but `Pattern`'s own array, so leaving
it in the REPL meant `repl.hpp` hand-looping over the public `tracks` field — knowing the
container's shape in order to ask it a question. Same reasoning that already puts
`bpm()`/`setBpm()` on the type rather than leaving callers to poke at `bpm_`.

The line this draws: `Pattern` may know *how* to find a track by name, but still not *which*
names exist. The v1 voice set stays in `makeDefaultPattern()` in `repl.hpp` — that is
application policy, and the earlier entry on keeping it out of `pattern.hpp` still holds.
Lookup is container mechanics, not policy.

Returns a raw `Track*`, null when nothing matches, which pairs with the
`if (Track* track = pattern.findTrack(command))` declaration-in-a-condition at the call site.
The array is fixed-size, so the pointer cannot be invalidated by the container growing.
`std::optional<std::reference_wrapper<Track>>` expresses the same thing with more ceremony,
and returning an index would push re-indexing back onto the caller. No `const` overload
until a const caller actually exists — today it would be a duplicated body bought for
nothing.

## 2026-09-11 — The REPL joins whitespace-separated step groups; `parseSteps` stays strict

`kick xxxx xxxx xxxx xxxx` — grouping in fours is a common tracker habit — previously read
only the first group and reported a confusing length error. `runRepl` now consumes every
remaining token on the line and concatenates them, so whitespace inside a pattern simply
falls out and `kick x..x..x..x..x..x` still works unchanged.

The joining happens in the REPL, not in `parseSteps`, which still demands exactly
`kStepsPerTrack` characters of `x`/`.`. Same split already recorded for `bpm`: forgiving at
the UI, strict at the type. `parseSteps` will eventually also be fed by JSON save files,
where a pattern with spaces in it means a corrupted file rather than a human being casual.

Consequence, accepted deliberately: trailing junk is no longer ignored.
`kick x..x..x..x..x..x junk` now joins to 20 characters and errors, where previously the junk
was silently dropped and the pattern applied. That differs from `quit junk`, which still
ignores its extra tokens — for a track line the remainder of the line *is* the argument, so
there is nothing extra to ignore. The length error now also mentions that spaces between
groups are ignored, so a miscounted group does not get told "must be 16 characters" while
the user is looking at four groups.

One side effect worth recording, because it makes an earlier entry's reasoning stale: the
2026-09-04 entry justified stripping the trailing `'\r'` by saying the next slice would read
the remainder of the line and hand `parseSteps` 17 characters on Windows line endings. That
slice is this one, and it reads the remainder with `operator>>` rather than verbatim, so
`'\r'` is skipped as whitespace and the strip is currently a no-op on every path. It stays
anyway: the first command that does consume the rest of a line verbatim — a file path for
JSON load, most likely — would otherwise break on CRLF input, and that failure would be
invisible on Linux.

## 2026-09-11 — `bpm` rejects out-of-range tempos rather than clamping, and needs no `try`/`catch`

`bpm` on its own prints the current tempo; `bpm <n>` sets it, accepting anything from 20 to
300 inclusive, fractional values included. `kMinBpm`/`kMaxBpm` live in `repl.hpp` beside
`kDefaultBpm`, not in `pattern.hpp`: what counts as a musically sane tempo is UI policy, while
`Pattern`'s own invariant stays the broader `bpm > 0`.

The 2026-09-01 entry floated clamping a human's bad tempo as the forgiving-UI half of the
split. Rejected in favour of an error that names the range and leaves the old tempo alone. A
typo like `bpm 1400` clamping to 300 silently gives the user a tempo they never asked for;
"forgiving" here means not crashing the session and not corrupting state, not guessing what
was meant. Rejection also matches what a bad step pattern already does.

This supersedes the 2026-09-04 prediction that the command "will catch `Pattern`'s
`std::invalid_argument`". It does not, because it cannot need to: the range check guarantees
`value >= 20`, so `validateBpm`'s throw is unreachable from this path, and a `catch` for it
would be exactly the defensive code CLAUDE.md rules out. `Pattern` stays strict regardless —
the guarantee is still enforced at the type, it is simply never the thing that reports a typo.
`bpm 0` is covered by a test specifically to pin that the range gate, not the exception, is
what turns it away.

The argument is read as one token and re-parsed through an `istringstream`, requiring
`eof()` so that `140abc` is rejected rather than quietly read as 140. Worth noting because it
is a real difference from `strtod`: `operator>>` into a `double` has no atom for `i` or `n`,
so `inf` and `nan` fail to parse outright and can never reach the range check as non-finite
values. Overflowing input like `1e400` sets failbit and is reported as "takes one number"
rather than as a range error — technically it is a number, but detecting that case would mean
inspecting failbit and `HUGE_VAL` together to improve the wording of a message nobody sane
will see.

## 2026-09-11 — `Oscillator`: a phase accumulator normalised to `[0, 1)`, wrapped by one add or subtract

Phase is tracked as a fraction of a cycle rather than in radians, and multiplied by 2π only at
the call to `std::sin`. Two reasons. Wrapping is then a subtraction of `1.0`, which is exactly
representable in binary and therefore lossless, where subtracting `2π` would fold that
constant's own representation error into the phase on every wrap. And a normalised phase is
waveform-agnostic — a sawtooth is `phase * 2 - 1`, a square is `phase < 0.5` — so the
accumulator survives v1's sine-only scope unchanged.

The phase is a `double` while samples are `float`. The phase is the only value here that is a
running total, so it is the only one where rounding accumulates; each sample is computed fresh
and discarded. Output is `float` because that is the buffer format miniaudio wants, and already
finer than the converter at the end of the chain resolves. Worth being honest that the drift a
`float` phase would cause is small — inaudible over a 16-step loop — so this is the cheap
general habit (accumulate wide, output narrow) rather than a fix for a measured problem.

`setFrequency` divides once and stores `phase_increment_`; `nextSample` only adds. The compiler
cannot hoist that division itself, because samples are pulled one at a time through `this` with
no loop in view, and `nextSample` sits on the audio callback's deadline where a division is
roughly ten times an add for an answer that does not change.

The wrap handles a negative phase as well as an overshooting one, so the precondition is only
that the frequency's magnitude stays below the sample rate — unbreakable in practice, since
half the sample rate is already the ceiling for a meaningful pitch. The negative case is
reachable: a drum voice's downward pitch sweep written the obvious way goes negative as soon as
the clock pulls samples past the sweep's nominal end.

Two corrections to the reasoning that led there, recorded because they are easy to get wrong
twice. The precision argument for the negative branch is weak — an escaping phase needs on the
order of 10⁹ samples before a `double` loses enough resolution to hear, so it would never bite
in practice. The argument that holds is the waveform one above: a sawtooth or square read from
a phase of −50000.25 is not slightly wrong but catastrophically wrong, where sine's periodicity
hides it entirely. And for the same reason the branch cannot currently be pinned by a test
through the public API: `sin` returns the right value either way. A `phase()` accessor would
make it testable and was rejected as public surface existing only for a test. Revisit when a
second waveform lands, which will make boundedness observable from outside.

`std::floor` was the alternative wrap and handles any value with no precondition at all. On a
target with SSE4.1 it is a single `roundsd`; on the baseline x86-64 we actually build for, GCC
expands it to roughly fifteen instructions with a branch and an integer conversion, against two
well-predicted compares for the hand-written version.

The constructor validates the sample rate and throws, matching `Pattern::validateBpm`. The
sample rate will come from a device config rather than user input, so this is not the
save-file argument that justifies `Pattern`'s strictness; it is a divide-by-zero guard, and
zero there poisons every later sample with NaN rather than failing anywhere near the cause.
`kTwoPi` is a private class constant rather than a namespace-scope one so that a later synth
header declaring its own cannot collide with it.

## 2026-09-15 — Each REPL command gets a handler taking the half-read line; dispatch is one `else if` chain

`runRepl` was 74 lines, alternating between loop plumbing (prompt, `getline`, CRLF strip,
tokenise) and the full argument-parsing implementation of whichever command matched. `bpm` was
21 of those lines and the track-steps branch 19, nested four deep inside `while (true)`. The
bodies moved to `detail::handleBpm` and `detail::handleSteps`, leaving a 37-line `runRepl` whose
loop body reads as read-normalise-dispatch with one line per command. Done before the synth
slice rather than after: `play` is the first command to own real state (an audio device), and
this is the shape that decides where that state lives. No test changed — the tests drive the
stream seam, not the internals, which is the evidence the refactor is behaviour-preserving.

The contract that comes with it: a handler receives the *partially consumed* line, after
`runRepl` has read the command token off the front, and owns whatever is left of it. So the
parameter is `std::istream&`, not a pre-parsed argument list and not the concrete
`std::istringstream&` — the handlers only ever need `operator>>`, and taking the base matches the
reason `runRepl` itself takes stream bases (2026-09-04). Pre-parsing into a `vector<string>` was
the alternative and was rejected: `handleSteps` wants every remaining token joined, `bpm` wants
exactly one and treats a second as an error, and a future `load` will want the rest of the line
verbatim as a path. There is no one tokenisation that serves all three, so the line is handed
over intact and each handler reads it its own way.

`handleSteps` takes `Track&` although `findTrack` returns `Track*`, dereferenced at the call
site inside the branch that already proved it non-null. The reference makes "non-null" a
guarantee in the signature rather than a comment — the payoff the 2026-09-11 `findTrack` entry
was setting up. It deliberately does *not* also take the name the user typed: `findTrack`
matched on `track.name == command`, so the two are equal by construction and a second parameter
could only ever drift out of step with the first. If lookup ever becomes case-insensitive or
aliased they stop being the same thing and the message should echo what was typed; that is the
moment to add it back, not now.

The `if (...) { ...; continue; }` chain became a single `if`/`else if`/`else`. Every old branch
already ended in `continue` or `return`, so the branches were mutually exclusive in effect but
not in structure, and the structure failed *open*: a forgotten `continue` fell through into the
track lookup and then printed "unknown command" after a command had in fact succeeded. Nothing
caught that but review. The chain cannot express it. `else` also replaces the trailing
unknown-command line, so the exhaustiveness is visible in one place.

Considered a `std::map<std::string_view, handler>` dispatch table and rejected at four commands.
`quit` has to stop the loop, so every handler would need a return value the other three ignore,
and the track-name branch is not a lookup by a known key at all — it is the fallback that runs
when none matched. Revisit around eight or ten commands, once `help`/`save`/`load` land.
