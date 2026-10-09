#include <iostream>

#include <stepseq/audio_device.hpp>
#include <stepseq/pattern.hpp>
#include <stepseq/repl.hpp>

namespace {

// Stand-in until the miniaudio device exists: accepts play/stop and makes no sound.
class SilentAudioDevice : public stepseq::AudioDevice {
public:
    void start() override {}
    void stop() override {}
};

}  // namespace

int main() {
    stepseq::Pattern pattern = stepseq::makeDefaultPattern();
    SilentAudioDevice device;
    stepseq::runRepl(std::cin, std::cout, pattern, device);

    return 0;
}
