#include "audio_sample.h"
#include "resource_selection.h"
#include "Resource/RZTexture.h"
#include <functional>

    uint16_t ReadBE16(const uint8_t* p)
    {
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }

    uint32_t ReadBE32(const uint8_t* p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }

    Xma2Format ParseXma2Format(const uint8_t* p, size_t avail)
    {
        Xma2Format f{};
        if (avail < 52)
            return f;
        f.formatTag = ReadBE16(p + 0);
        f.channels = ReadBE16(p + 2);
        f.samplesPerSec = ReadBE32(p + 4);
        f.avgBytesPerSec = ReadBE32(p + 8);
        f.blockAlign = ReadBE16(p + 12);
        f.bitsPerSample = ReadBE16(p + 14);
        f.cbSize = ReadBE16(p + 16);
        f.numStreams = ReadBE16(p + 18);
        f.channelMask = ReadBE32(p + 20);
        f.samplesEncoded = ReadBE32(p + 24);
        f.bytesPerBlock = ReadBE32(p + 28);
        f.playBegin = ReadBE32(p + 32);
        f.playLength = ReadBE32(p + 36);
        f.loopBegin = ReadBE32(p + 40);
        f.loopLength = ReadBE32(p + 44);
        f.loopCount = p[48];
        f.encoderVersion = p[49];
        f.blockCount = ReadBE16(p + 50);
        return f;
    }

    bool ExtractAudioPayload(const PKPackage& package, const CMChunk& chunk, Xma2Format& fmt, std::vector<uint8_t>& xmaData)
    {
        const uint32_t id = chunk.GetMaskedID();
        const uint8_t* d = chunk.data.data();
        const size_t n = chunk.data.size();

        if (id == CMChunkTypes::Audio_SampleData)
        {
            if (n < 80)
                return false;
            const uint32_t seekTableSize = ReadBE32(d + 4);
            const uint32_t dataSize = ReadBE32(d + 24);
            fmt = ParseXma2Format(d + 28, n - 28);
            const size_t dataOff = 80 + static_cast<size_t>(seekTableSize);
            if (dataOff > n || dataSize > n - dataOff)
                return false;
            xmaData.assign(d + dataOff, d + dataOff + dataSize);
            return fmt.formatTag == 0x0166 && fmt.channels > 0 && fmt.samplesPerSec > 0 && !xmaData.empty();
        }

        if (id == CMChunkTypes::CutScene_Audio_SampleData)
        {
            if (n < 20)
                return false;
            const uint32_t dataSize = ReadBE32(d + 16);
            if (dataSize > n - 20)
                return false;
            xmaData.assign(d + 20, d + 20 + dataSize);

            const CMChunk* hdr = nullptr;
            if (!package.rootChunks.empty())
            {
                for (const CMChunk& root : package.rootChunks)
                {
                    std::function<const CMChunk* (const CMChunk&)> find = [&](const CMChunk& c) -> const CMChunk* {
                        if (c.GetMaskedID() == CMChunkTypes::CutScene_Audio_SampleHeader)
                            return &c;
                        for (const CMChunk& ch : c.children)
                        {
                            if (const CMChunk* f = find(ch))
                                return f;
                        }
                        return nullptr;
                        };
                    hdr = find(root);
                    if (hdr)
                        break;
                }
            }
            if (!hdr || hdr->data.size() < 76)
                return false;
            fmt = ParseXma2Format(hdr->data.data() + 24, hdr->data.size() - 24);
            return fmt.formatTag == 0x0166 && fmt.channels > 0 && fmt.samplesPerSec > 0 && !xmaData.empty();
        }

        return false;
    }

    void WriteLE16(std::vector<uint8_t>& b, uint16_t v)
    {
        b.push_back(static_cast<uint8_t>(v & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    void WriteLE32(std::vector<uint8_t>& b, uint32_t v)
    {
        b.push_back(static_cast<uint8_t>(v & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }

    std::vector<uint8_t> BuildXmaContainer(const Xma2Format& fmt, const uint8_t* data, size_t size)
    {
        std::vector<uint8_t> out;
        out.reserve(12 + 8 + 52 + 8 + size);
        out.insert(out.end(), { 'R', 'I', 'F', 'F' });
        WriteLE32(out, 0);
        out.insert(out.end(), { 'W', 'A', 'V', 'E' });
        out.insert(out.end(), { 'f', 'm', 't', ' ' });
        WriteLE32(out, 52);
        WriteLE16(out, fmt.formatTag);
        WriteLE16(out, fmt.channels);
        WriteLE32(out, fmt.samplesPerSec);
        WriteLE32(out, fmt.avgBytesPerSec);
        WriteLE16(out, fmt.blockAlign);
        WriteLE16(out, fmt.bitsPerSample);
        WriteLE16(out, fmt.cbSize ? fmt.cbSize : 34);
        WriteLE16(out, fmt.numStreams);
        WriteLE32(out, fmt.channelMask);
        WriteLE32(out, fmt.samplesEncoded);
        WriteLE32(out, fmt.bytesPerBlock);
        WriteLE32(out, fmt.playBegin);
        WriteLE32(out, fmt.playLength);
        WriteLE32(out, fmt.loopBegin);
        WriteLE32(out, fmt.loopLength);
        out.push_back(fmt.loopCount);
        out.push_back(fmt.encoderVersion);
        WriteLE16(out, fmt.blockCount);
        out.insert(out.end(), { 'd', 'a', 't', 'a' });
        WriteLE32(out, static_cast<uint32_t>(size));
        out.insert(out.end(), data, data + size);
        const uint32_t riffSize = static_cast<uint32_t>(out.size() - 8);
        out[4] = static_cast<uint8_t>(riffSize & 0xFF);
        out[5] = static_cast<uint8_t>((riffSize >> 8) & 0xFF);
        out[6] = static_cast<uint8_t>((riffSize >> 16) & 0xFF);
        out[7] = static_cast<uint8_t>((riffSize >> 24) & 0xFF);
        return out;
    }


AudioSample SelectedAudio(const PackageSession& session)
{
    if (!session.package || !session.selected) throw std::runtime_error("Select an audio sample resource first.");
    AudioSample sample;
    const CMChunk* chunk = session.selected;
    const auto header = SelectedResource(*session.package, session.selected);
    if (header && header->GetResourceType() == kCMResourceType_AudioSample)
    {
        const CMChunk* post = RZTexture::FindRootAtOffset(*session.package, header->GetDataOffset());
        chunk = post && post->GetMaskedID() == Gen_PostLoadData ? post->FindChild(Audio_SampleData) : nullptr;
        sample.name = header->GetName();
    }
    if (!chunk || !ExtractAudioPayload(*session.package, *chunk, sample.format, sample.data))
        throw std::runtime_error("Select a valid XMA2 audio sample resource first.");
    if (sample.name.empty()) sample.name = "sample";
    return sample;
}
