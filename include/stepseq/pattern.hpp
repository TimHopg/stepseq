#pragma once

#include <array>
#include <stdexcept>
#include <utility>

#include <stepseq/track.hpp>

namespace stepseq {

inline constexpr std::size_t kTracksPerPattern = 4;

class Pattern {
public:
    Pattern(double bpm, std::array<Track, kTracksPerPattern> tracks)
        : tracks(std::move(tracks)), bpm_(validateBpm(bpm)) {}

    double bpm() const { return bpm_; }
    void setBpm(double bpm) { bpm_ = validateBpm(bpm); }

    std::array<Track, kTracksPerPattern> tracks;

private:
    static double validateBpm(double bpm) {
        if (bpm <= 0.0) {
            throw std::invalid_argument("Pattern bpm must be positive");
        }
        return bpm;
    }

    double bpm_;
};

} // namespace stepseq
