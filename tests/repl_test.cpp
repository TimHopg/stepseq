#include <catch2/catch_test_macros.hpp>

#include <array>
#include <sstream>
#include <string>
#include <utility>

#include <stepseq/pattern.hpp>
#include <stepseq/repl.hpp>
#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace {

// Expected output is kBanner + (kPrompt + command output)... + kEofTail, or
// nothing after the prompt for quit/exit. One test below pins the banner text.
const std::string kBanner{stepseq::kBanner};
const std::string kPrompt = "> ";

// The prompt before the read that hits end-of-input, plus its closing newline.
const std::string kEofTail = "> \n";

// What `print` renders for a freshly-made test pattern with no active steps.
const std::string kEmptyPatternOutput =
    "bpm:    120\n"
    "kick:   ................\n"
    "snare:  ................\n"
    "hat:    ................\n"
    "synth:  ................\n";

// Deliberately not makeDefaultPattern(): these tests pin exact output, so
// changing the v1 voice set should not break command-loop tests.
stepseq::Pattern makeTestPattern() {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "kick";
    tracks[1].name = "snare";
    tracks[2].name = "hat";
    tracks[3].name = "synth";
    return stepseq::Pattern(120.0, std::move(tracks));
}

std::string runReplOn(const std::string& input, stepseq::Pattern& pattern) {
    std::istringstream in(input);
    std::ostringstream out;
    stepseq::runRepl(in, out, pattern);
    return out.str();
}

} // namespace

TEST_CASE("makeDefaultPattern builds the v1 voice set at the default tempo") {
    const stepseq::Pattern pattern = stepseq::makeDefaultPattern();

    // Spelled out rather than compared against kDefaultBpm, so changing the
    // default has to be a deliberate decision.
    REQUIRE(pattern.bpm() == 120.0);
    REQUIRE(pattern.tracks[0].name == "kick");
    REQUIRE(pattern.tracks[1].name == "snare");
    REQUIRE(pattern.tracks[2].name == "hat");
    REQUIRE(pattern.tracks[3].name == "synth");

    for (const stepseq::Track& track : pattern.tracks) {
        for (const stepseq::Step& step : track.steps) {
            REQUIRE_FALSE(step.active);
        }
    }
}

TEST_CASE("runRepl opens with the banner, before the first prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\n", pattern) ==
            "stepseq - 'kick x..x..x..x..x..x' sets steps, 'print' shows the "
            "pattern, 'quit' exits.\n"
            "> ");
}

TEST_CASE("runRepl returns on quit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl returns on exit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("exit\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl stops reading once it sees quit") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\nbogus\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl closes the dangling prompt line at end of input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("", pattern) == kBanner + kEofTail);
}

TEST_CASE("runRepl handles a final line with no trailing newline") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl strips the trailing carriage return from CRLF input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\r\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("bogus\r\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: bogus\n" + kEofTail);
}

TEST_CASE("runRepl skips blank and whitespace-only lines") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("\n   \n\t\n", pattern) ==
            kBanner + kPrompt + kPrompt + kPrompt + kEofTail);
}

TEST_CASE("runRepl reports an unknown command and keeps going") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bogus\nalsobogus\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: bogus\n" + kPrompt +
                "error: unknown command: alsobogus\n" + kEofTail);
}

TEST_CASE("runRepl keeps going after a successful command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\nbogus\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kPrompt +
                "error: unknown command: bogus\n" + kEofTail);
}

TEST_CASE("runRepl commands are case-sensitive") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("PRINT\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: PRINT\n" + kEofTail);
    REQUIRE(runReplOn("Quit\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: Quit\n" + kEofTail);
}

TEST_CASE("runRepl ignores surrounding whitespace around a command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("   print   \n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl ignores extra tokens after a command that takes no arguments") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print junk\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("quit junk\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("a track name followed by a step pattern sets that track's steps") {
    stepseq::Pattern pattern = makeTestPattern();

    // Silent on success; `print` is the way to see the result.
    REQUIRE(runReplOn("kick x..x..x..x..x..x\n", pattern) ==
            kBanner + kPrompt + kEofTail);
    REQUIRE(pattern.tracks[0].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[1].active);
    REQUIRE(pattern.tracks[0].steps[3].active);
    REQUIRE(pattern.tracks[0].steps[15].active);
}

TEST_CASE("a track line only touches the named track") {
    stepseq::Pattern pattern = makeTestPattern();

    runReplOn("hat xxxxxxxxxxxxxxxx\n", pattern);

    REQUIRE(pattern.tracks[2].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[1].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[3].steps[0].active);
}

TEST_CASE("a track line replaces the previous steps, not merges with them") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.tracks[0].steps[1].active = true;

    runReplOn("kick x...............\n", pattern);

    REQUIRE(pattern.tracks[0].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[1].active);
}

TEST_CASE("a track name with no pattern prints a usage error") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick\n", pattern) ==
            kBanner + kPrompt +
                "error: 'kick' needs a step pattern, e.g. 'kick x..x..x..x..x..x'\n" +
                kEofTail);
}

TEST_CASE("a bad step pattern is reported and leaves the track unchanged") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.tracks[0].steps[5].active = true;

    const std::string wrong_length = runReplOn("kick x..x\n", pattern);
    const std::string bad_char = runReplOn("kick x..q..x..x..x..x\n", pattern);

    const std::string expected = kBanner + kPrompt +
        "error: step pattern must be 16 steps of 'x' or '.' (spaces between groups are ignored)\n" + kEofTail;
    REQUIRE(wrong_length == expected);
    REQUIRE(bad_char == expected);
    REQUIRE(pattern.tracks[0].steps[5].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[0].active);
}

TEST_CASE("track names are case-sensitive, like commands") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("Kick x..x..x..x..x..x\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: Kick\n" + kEofTail);
}

TEST_CASE("step groups separated by spaces are joined into one pattern") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick xxxx .... xx.. ..xx\n", pattern) ==
            kBanner + kPrompt + kEofTail);
    REQUIRE(pattern.tracks[0].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[4].active);
    REQUIRE(pattern.tracks[0].steps[8].active);
    REQUIRE(pattern.tracks[0].steps[14].active);
}

TEST_CASE("a track line rejects trailing junk, since the tokens are joined") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick x..x..x..x..x..x junk\n", pattern) ==
            kBanner + kPrompt +
                "error: step pattern must be 16 steps of 'x' or '.' (spaces between groups are ignored)\n" +
                kEofTail);
    REQUIRE_FALSE(pattern.tracks[0].steps[0].active);
}

TEST_CASE("grouped steps of the wrong total length are still rejected") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick xxx xxxx xxxx xxxx\n", pattern) ==
            kBanner + kPrompt +
                "error: step pattern must be 16 steps of 'x' or '.' (spaces between groups are ignored)\n" +
                kEofTail);
    REQUIRE_FALSE(pattern.tracks[0].steps[0].active);
}

TEST_CASE("step groups separated by tabs are joined too") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick xxxx\t....\txxxx\t....\n", pattern) ==
            kBanner + kPrompt + kEofTail);
    REQUIRE(pattern.tracks[0].steps[0].active);
    REQUIRE_FALSE(pattern.tracks[0].steps[4].active);
    REQUIRE(pattern.tracks[0].steps[8].active);
}

TEST_CASE("a track name followed by only whitespace prints the usage error") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("kick   \n", pattern) ==
            kBanner + kPrompt +
                "error: 'kick' needs a step pattern, e.g. 'kick x..x..x..x..x..x'\n" +
                kEofTail);
}

TEST_CASE("a track named after a built-in cannot shadow the command") {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "print";
    stepseq::Pattern pattern(120.0, std::move(tracks));

    const std::string output = runReplOn("print xxxxxxxxxxxxxxxx\n", pattern);

    // The built-in ran (and ignored the junk); the track was not written.
    REQUIRE(output.find("bpm:") != std::string::npos);
    REQUIRE_FALSE(pattern.tracks[0].steps[0].active);
}

TEST_CASE("print renders the bpm and every track") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("print renders active steps as 'x'") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.tracks[0].steps[0].active = true;
    pattern.tracks[0].steps[4].active = true;

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("kick:   x...x...........\n") != std::string::npos);
}

TEST_CASE("print reflects a changed bpm") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.setBpm(140.0);

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("bpm:    140\n") != std::string::npos);
}

TEST_CASE("printPattern can be called directly on a const Pattern") {
    const stepseq::Pattern pattern = makeTestPattern();
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    REQUIRE(out.str() == kEmptyPatternOutput);
}

TEST_CASE("printPattern starts every row's value at the same column") {
    // Names chosen to span the interesting lengths: comfortably short, exactly
    // at kMaxLabelNameWidth, long enough to abridge, and empty by default.
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "hat";
    tracks[1].name = "cowbel";
    tracks[2].name = "resonator";
    stepseq::Pattern pattern(120.0, std::move(tracks));
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    std::istringstream lines(out.str());
    std::string line;
    std::size_t rows = 0;
    while (std::getline(lines, line)) {
        REQUIRE(line.find_first_not_of(' ', line.find(':') + 1) ==
                stepseq::kLabelWidth);
        ++rows;
    }
    // Without this the loop would vacuously pass on empty output.
    REQUIRE(rows == 1 + stepseq::kTracksPerPattern);
}

TEST_CASE("printPattern abridges a name too long for the label column") {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "resonator";
    stepseq::Pattern pattern(120.0, std::move(tracks));
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    // kMaxLabelNameWidth characters, then ':' and the one space that is left.
    REQUIRE(out.str().find("resona: ................\n") != std::string::npos);
}

TEST_CASE("printPattern renders a nameless track as a bare label") {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    stepseq::Pattern pattern(120.0, std::move(tracks));
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    // Nameless tracks are neither skipped nor given a placeholder.
    REQUIRE(out.str().find(":       ................\n") != std::string::npos);
}

TEST_CASE("bpm with a number sets the tempo") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bpm 140\n", pattern) == kBanner + kPrompt + kEofTail);
    REQUIRE(pattern.bpm() == 140.0);
}

TEST_CASE("bpm accepts a fractional tempo") {
    stepseq::Pattern pattern = makeTestPattern();

    runReplOn("bpm 128.5\n", pattern);

    REQUIRE(pattern.bpm() == 128.5);
}

TEST_CASE("bpm accepts both ends of the allowed range") {
    stepseq::Pattern pattern = makeTestPattern();

    runReplOn("bpm 20\n", pattern);
    REQUIRE(pattern.bpm() == 20.0);

    runReplOn("bpm 300\n", pattern);
    REQUIRE(pattern.bpm() == 300.0);
}

TEST_CASE("bare bpm reports the current tempo") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bpm\n", pattern) ==
            kBanner + kPrompt + "bpm:    120\n" + kEofTail);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("a tempo outside the musical range is rejected, leaving the old one") {
    stepseq::Pattern pattern = makeTestPattern();

    const std::string too_slow = runReplOn("bpm 5\n", pattern);
    const std::string too_fast = runReplOn("bpm 301\n", pattern);
    const std::string negative = runReplOn("bpm -140\n", pattern);

    const std::string expected =
        kBanner + kPrompt + "error: bpm must be between 20 and 300\n" + kEofTail;
    REQUIRE(too_slow == expected);
    REQUIRE(too_fast == expected);
    REQUIRE(negative == expected);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("a bpm argument that is not a number is rejected") {
    stepseq::Pattern pattern = makeTestPattern();

    const std::string letters = runReplOn("bpm abc\n", pattern);
    const std::string trailing = runReplOn("bpm 140abc\n", pattern);

    const std::string expected = kBanner + kPrompt +
        "error: 'bpm' takes one number, e.g. 'bpm 140'\n" + kEofTail;
    REQUIRE(letters == expected);
    REQUIRE(trailing == expected);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("bpm with extra tokens after the number is rejected") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bpm 140 junk\n", pattern) ==
            kBanner + kPrompt +
                "error: 'bpm' takes one number, e.g. 'bpm 140'\n" + kEofTail);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("a tempo set with bpm shows up in print") {
    stepseq::Pattern pattern = makeTestPattern();

    const std::string output = runReplOn("bpm 90\nprint\n", pattern);

    REQUIRE(output.find("bpm:    90\n") != std::string::npos);
}

TEST_CASE("bpm 0 is rejected by the range check, never reaching Pattern") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bpm 0\n", pattern) ==
            kBanner + kPrompt + "error: bpm must be between 20 and 300\n" + kEofTail);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("a tempo just outside the range is rejected") {
    stepseq::Pattern pattern = makeTestPattern();

    const std::string just_slow = runReplOn("bpm 19.9\n", pattern);
    const std::string just_fast = runReplOn("bpm 300.1\n", pattern);

    const std::string expected =
        kBanner + kPrompt + "error: bpm must be between 20 and 300\n" + kEofTail;
    REQUIRE(just_slow == expected);
    REQUIRE(just_fast == expected);
    REQUIRE(pattern.bpm() == 120.0);
}

TEST_CASE("nan and inf are not accepted as tempos") {
    stepseq::Pattern pattern = makeTestPattern();

    // operator>> into a double has no atom for 'n' or 'i', so these fail to
    // parse outright rather than arriving as a non-finite value.
    const std::string not_a_number = runReplOn("bpm nan\n", pattern);
    const std::string infinity = runReplOn("bpm inf\n", pattern);

    const std::string expected = kBanner + kPrompt +
        "error: 'bpm' takes one number, e.g. 'bpm 140'\n" + kEofTail;
    REQUIRE(not_a_number == expected);
    REQUIRE(infinity == expected);
    REQUIRE(pattern.bpm() == 120.0);
}
