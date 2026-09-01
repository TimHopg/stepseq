#include <catch2/catch_test_macros.hpp>

#include <stepseq/pattern.hpp>

TEST_CASE("kTracksPerPattern is 4") {
    REQUIRE(stepseq::kTracksPerPattern == 4);
}

TEST_CASE("a Pattern stores its bpm and tracks") {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "kick";

    const stepseq::Pattern pattern(120.0, tracks);

    REQUIRE(pattern.bpm() == 120.0);
    REQUIRE(pattern.tracks[0].name == "kick");
}

TEST_CASE("a Pattern rejects a non-positive bpm") {
    const std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};

    REQUIRE_THROWS_AS(stepseq::Pattern(0.0, tracks), std::invalid_argument);
    REQUIRE_THROWS_AS(stepseq::Pattern(-10.0, tracks), std::invalid_argument);
}
