#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace stepseq {

// A sine oscillator driven by a phase accumulator. A negative frequency is fine
// (a pitch sweep can overshoot); one at or above the sample rate is not.
class Oscillator {
public:
    explicit Oscillator(double sample_rate)
        : sample_rate_(validateSampleRate(sample_rate)) {}

    void setFrequency(double frequency) { phase_increment_ = frequency / sample_rate_; }

    void resetPhase() { phase_ = 0.0; }

    float nextSample() {
        const float sample = static_cast<float>(std::sin(kTwoPi * phase_));
        phase_ += phase_increment_;
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
        } else if (phase_ < 0.0) {
            phase_ += 1.0;
        }
        return sample;
    }

private:
    static constexpr double kTwoPi = 2.0 * std::numbers::pi;

    static double validateSampleRate(double sample_rate) {
        if (sample_rate <= 0.0) {
            throw std::invalid_argument("Oscillator sample rate must be positive");
        }
        return sample_rate;
    }

    double sample_rate_;
    double phase_increment_ = 0.0;
    double phase_ = 0.0;
};

} // namespace stepseq
