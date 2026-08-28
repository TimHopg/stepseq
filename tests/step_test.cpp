#include <catch2/catch_test_macros.hpp>

#include <stepseq/step.hpp>

TEST_CASE("a default-constructed Step is inactive") {
    const stepseq::Step step;
    REQUIRE_FALSE(step.active);
}
