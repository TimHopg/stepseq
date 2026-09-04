#include <catch2/catch_test_macros.hpp>

#include <array>
#include <sstream>
#include <string>

#include <stepseq/pattern.hpp>
#include <stepseq/repl.hpp>
#include <stepseq/track.hpp>

namespace {

// runRepl prints a prompt before every read, so expected output is built as:
// kPrompt + <output of command 1> + kPrompt + <output of command 2> + ... and
// then either kEofTail (input ran out) or nothing more (a quit/exit command).
const std::string kPrompt = "> ";

// The final prompt, printed before the read that hits end-of-input, plus the
// newline runRepl emits to close that dangling line.
const std::string kEofTail = "> \n";

// What `print` renders for a freshly-made test pattern with no active steps.
const std::string kEmptyPatternOutput =
    "bpm: 120\n"
    "kick: ................\n"
    "snare: ................\n"
    "hat: ................\n"
    "synth: ................\n";

stepseq::Pattern makeTestPattern() {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "kick";
    tracks[1].name = "snare";
    tracks[2].name = "hat";
    tracks[3].name = "synth";
    return stepseq::Pattern(120.0, tracks);
}

std::string runReplOn(const std::string& input, stepseq::Pattern& pattern) {
    std::istringstream in(input);
    std::ostringstream out;
    stepseq::runRepl(in, out, pattern);
    return out.str();
}

} // namespace

TEST_CASE("runRepl returns on quit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\n", pattern) == kPrompt);
}

TEST_CASE("runRepl returns on exit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("exit\n", pattern) == kPrompt);
}

TEST_CASE("runRepl stops reading once it sees quit") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\nbogus\n", pattern) == kPrompt);
}

TEST_CASE("runRepl closes the dangling prompt line at end of input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("", pattern) == kEofTail);
}

TEST_CASE("runRepl handles a final line with no trailing newline") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print", pattern) == kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl strips the trailing carriage return from CRLF input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\r\n", pattern) == kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("bogus\r\n", pattern) ==
            kPrompt + "error: unknown command: bogus\n" + kEofTail);
}

TEST_CASE("runRepl skips blank and whitespace-only lines") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("\n   \n\t\n", pattern) == kPrompt + kPrompt + kPrompt + kEofTail);
}

TEST_CASE("runRepl reports an unknown command and keeps going") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bogus\nalsobogus\n", pattern) ==
            kPrompt + "error: unknown command: bogus\n" +
                kPrompt + "error: unknown command: alsobogus\n" + kEofTail);
}

TEST_CASE("runRepl keeps going after a successful command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\nbogus\n", pattern) ==
            kPrompt + kEmptyPatternOutput + kPrompt + "error: unknown command: bogus\n" +
                kEofTail);
}

TEST_CASE("runRepl commands are case-sensitive") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("PRINT\n", pattern) ==
            kPrompt + "error: unknown command: PRINT\n" + kEofTail);
    REQUIRE(runReplOn("Quit\n", pattern) ==
            kPrompt + "error: unknown command: Quit\n" + kEofTail);
}

TEST_CASE("runRepl ignores surrounding whitespace around a command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("   print   \n", pattern) == kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl ignores extra tokens after a command that takes no arguments") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print junk\n", pattern) == kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("quit junk\n", pattern) == kPrompt);
}

TEST_CASE("print renders the bpm and every track") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\n", pattern) == kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("print renders active steps as 'x'") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.tracks[0].steps[0].active = true;
    pattern.tracks[0].steps[4].active = true;

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("kick: x...x...........\n") != std::string::npos);
}

TEST_CASE("print reflects a changed bpm") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.setBpm(140.0);

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("bpm: 140\n") != std::string::npos);
}

TEST_CASE("printPattern can be called directly on a const Pattern") {
    const stepseq::Pattern pattern = makeTestPattern();
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    REQUIRE(out.str() == kEmptyPatternOutput);
}
