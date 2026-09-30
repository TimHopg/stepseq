#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <type_traits>

#include <stepseq/audio_device.hpp>

#include "fake_audio_device.hpp"

namespace {

using stepseq::AudioDevice;
using stepseq::testing::FakeAudioDevice;

// Compile-time checks: the virtual destructor and deleted copies can't be dropped unnoticed.
static_assert(std::is_abstract_v<AudioDevice>);
static_assert(std::has_virtual_destructor_v<AudioDevice>);
static_assert(!std::is_copy_constructible_v<AudioDevice>);
static_assert(!std::is_copy_assignable_v<AudioDevice>);

TEST_CASE("calls through an AudioDevice reference reach the derived device", "[audio_device]") {
    FakeAudioDevice fake;
    AudioDevice& device = fake;

    device.start();
    device.start();
    device.stop();

    CHECK(fake.startCalls() == 2);
    CHECK(fake.stopCalls() == 1);
}

TEST_CASE("deleting through an AudioDevice pointer runs the derived destructor", "[audio_device]") {
    bool destroyed = false;
    {
        std::unique_ptr<AudioDevice> device = std::make_unique<FakeAudioDevice>(&destroyed);
        CHECK_FALSE(destroyed);
    }
    CHECK(destroyed);
}

}  // namespace
