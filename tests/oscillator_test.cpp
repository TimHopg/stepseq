#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include <stepseq/synth/oscillator.hpp>

namespace {

// Samples sit in [-1, 1], so compare against an absolute margin: Approx's
// default relative epsilon collapses to exact equality at the zero crossings.
Catch::Approx near(double expected) {
    return Catch::Approx(expected).margin(1e-6);
}

double sineAt(double phase) {
    return std::sin(2.0 * std::numbers::pi * phase);
}

} // namespace

TEST_CASE("an Oscillator rejects a non-positive sample rate") {
    REQUIRE_THROWS_AS(stepseq::Oscillator(0.0), std::invalid_argument);
    REQUIRE_THROWS_AS(stepseq::Oscillator(-44100.0), std::invalid_argument);
}

TEST_CASE("an Oscillator with no frequency set is silent") {
    stepseq::Oscillator oscillator(44100.0);

    for (int i = 0; i < 16; ++i) {
        REQUIRE(oscillator.nextSample() == near(0.0));
    }
}

TEST_CASE("nextSample walks a sine wave from phase zero") {
    // Four samples per second at 1 Hz puts one sample on each quarter cycle.
    stepseq::Oscillator oscillator(4.0);
    oscillator.setFrequency(1.0);

    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(1.0));
    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(-1.0));
}

TEST_CASE("a negative frequency runs the wave backwards and keeps wrapping") {
    stepseq::Oscillator oscillator(4.0);
    oscillator.setFrequency(-1.0);

    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(-1.0));
    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(1.0));

    // This pins that a negative frequency plays the wave backwards, not that the
    // phase stays in [0, 1): sine is periodic, so an escaping phase reads the same.
    for (int i = 0; i < 4 * 10000; ++i) {
        oscillator.nextSample();
    }

    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(-1.0));
}

TEST_CASE("the frequency stays stable over hundreds of cycles") {
    // Note this does not pin the wrap itself: sine is periodic, so deleting the
    // wrap leaves a phase near 500 that still produces these values.
    constexpr int kSamplesPerCycle = 100;
    stepseq::Oscillator oscillator(1000.0);
    oscillator.setFrequency(10.0);

    std::vector<float> first_cycle;
    for (int i = 0; i < kSamplesPerCycle; ++i) {
        first_cycle.push_back(oscillator.nextSample());
    }
    for (int i = 0; i < kSamplesPerCycle * 499; ++i) {
        oscillator.nextSample();
    }

    for (int i = 0; i < kSamplesPerCycle; ++i) {
        REQUIRE(oscillator.nextSample() == near(first_cycle[i]));
    }
}

TEST_CASE("a realistic rate wraps between samples without drifting") {
    // Every other rate here divides exactly into its frequency, so the wrap lands
    // on a sample boundary. 440 at 44100 is the arithmetic that actually ships.
    constexpr double kSampleRate = 44100.0;
    constexpr double kFrequency = 440.0;
    constexpr int kOneSecond = 44100;
    constexpr int kSamplesPerCheck = 100;
    stepseq::Oscillator oscillator(kSampleRate);
    oscillator.setFrequency(kFrequency);

    for (int i = 0; i < kOneSecond; ++i) {
        oscillator.nextSample();
    }

    for (int i = kOneSecond; i < kOneSecond + kSamplesPerCheck; ++i) {
        const double phase = std::fmod(i * kFrequency / kSampleRate, 1.0);
        REQUIRE(oscillator.nextSample() == near(sineAt(phase)));
    }
}

TEST_CASE("resetPhase restarts the wave") {
    stepseq::Oscillator oscillator(4.0);
    oscillator.setFrequency(1.0);
    oscillator.nextSample();
    oscillator.nextSample();

    oscillator.resetPhase();

    REQUIRE(oscillator.nextSample() == near(0.0));
    REQUIRE(oscillator.nextSample() == near(1.0));
}

TEST_CASE("setFrequency changes the wave from the next sample on") {
    stepseq::Oscillator oscillator(4.0);
    oscillator.setFrequency(1.0);
    REQUIRE(oscillator.nextSample() == near(0.0));

    oscillator.setFrequency(2.0);

    // Phase is already at 0.25 and now advances by half a cycle at a time.
    REQUIRE(oscillator.nextSample() == near(1.0));
    REQUIRE(oscillator.nextSample() == near(-1.0));
}
