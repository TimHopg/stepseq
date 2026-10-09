#pragma once

#include <stepseq/audio_device.hpp>

namespace stepseq::testing {

// Records what it was asked to do instead of touching any sound hardware.
class FakeAudioDevice : public AudioDevice {
public:
    FakeAudioDevice() = default;
    explicit FakeAudioDevice(bool* destroyed) : destroyed_(destroyed) {}
    ~FakeAudioDevice() override {
        if (destroyed_ != nullptr) {
            *destroyed_ = true;
        }
    }

    void start() override { ++start_calls_; }
    void stop() override { ++stop_calls_; }

    int startCalls() const { return start_calls_; }
    int stopCalls() const { return stop_calls_; }

private:
    int start_calls_ = 0;
    int stop_calls_ = 0;
    bool* destroyed_ = nullptr;
};

}  // namespace stepseq::testing
