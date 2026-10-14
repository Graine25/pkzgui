#pragma once
#include "audio_sample.h"
#include <memory>

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();
    bool IsBusy() const;
    void Begin(AudioSample sample, const std::string& exportPath = {});
    void Poll(std::string& status, std::string& error);
    void Stop();
private:
    struct State;
    std::unique_ptr<State> state;
};
