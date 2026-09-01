#pragma once

#include <array>
#include <stdexcept>
#include <string_view>

#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace stepseq {

inline std::array<Step, kStepsPerTrack> parseSteps(std::string_view pattern) {
    if (pattern.size() != kStepsPerTrack) {
        throw std::invalid_argument("pattern must have exactly kStepsPerTrack characters");
    }

    std::array<Step, kStepsPerTrack> steps{};
    for (std::size_t i = 0; i < kStepsPerTrack; ++i) {
        const char c = pattern[i];
        if (c == 'x') {
            steps[i].active = true;
        } else if (c == '.') {
            steps[i].active = false;
        } else {
            throw std::invalid_argument("pattern characters must be 'x' or '.'");
        }
    }

    return steps;
}

} // namespace stepseq
