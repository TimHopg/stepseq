#pragma once

namespace stepseq {

// What the REPL may ask of a sound device; the real miniaudio one and test fakes implement it.
class AudioDevice {
public:
    AudioDevice() = default;
    virtual ~AudioDevice() = default;

    // Copying through a base reference would slice the derived device.
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    virtual void start() = 0;
    virtual void stop() = 0;
};

}  // namespace stepseq
