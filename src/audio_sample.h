#pragma once
#include "package_session.h"

struct Xma2Format
    {
        uint16_t formatTag = 0;
        uint16_t channels = 0;
        uint32_t samplesPerSec = 0;
        uint32_t avgBytesPerSec = 0;
        uint16_t blockAlign = 0;
        uint16_t bitsPerSample = 0;
        uint16_t cbSize = 0;
        uint16_t numStreams = 0;
        uint32_t channelMask = 0;
        uint32_t samplesEncoded = 0;
        uint32_t bytesPerBlock = 0;
        uint32_t playBegin = 0;
        uint32_t playLength = 0;
        uint32_t loopBegin = 0;
        uint32_t loopLength = 0;
        uint8_t loopCount = 0;
        uint8_t encoderVersion = 0;
        uint16_t blockCount = 0;
    };

struct AudioSample { Xma2Format format; std::vector<uint8_t> data; std::string name; };
AudioSample SelectedAudio(const PackageSession& session);
std::vector<uint8_t> BuildXmaContainer(const Xma2Format& fmt, const uint8_t* data, size_t size);
