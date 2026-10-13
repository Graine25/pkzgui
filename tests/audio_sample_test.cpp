#include "audio_sample.h"
#include "audio_process.h"
#include "file_io.h"
#include <iostream>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Put16(std::vector<uint8_t>& bytes, size_t at, uint16_t value)
{ bytes[at] = value >> 8; bytes[at + 1] = value; }
void Put32(std::vector<uint8_t>& bytes, size_t at, uint32_t value)
{ for (int i = 0; i < 4; ++i) bytes[at + i] = value >> (24 - 8 * i); }

int main()
{
    try
    {
        PackageSession session;
        session.package = std::make_unique<PKPackage>();
        std::vector<uint8_t> payload(87, 0);
        Put32(payload, 4, 4);
        Put32(payload, 24, 3);
        Put16(payload, 28, 0x166);
        Put16(payload, 30, 2);
        Put32(payload, 32, 48000);
        Put16(payload, 42, 16);
        Put16(payload, 46, 1);
        payload[84] = 1; payload[85] = 2; payload[86] = 3;
        session.package->rootChunks.push_back(CMChunk::Leaf(Audio_SampleData, 1, payload));
        session.selected = &session.package->rootChunks[0];
        const auto sample = SelectedAudio(session);
        Check(sample.format.channels == 2 && sample.format.samplesPerSec == 48000 && sample.data == std::vector<uint8_t>({1, 2, 3}), "Wrong XMA payload/format");
        const auto wave = BuildXmaContainer(sample.format, sample.data.data(), sample.data.size());
        Check(wave.size() == 83 && std::string(wave.begin(), wave.begin() + 4) == "RIFF", "RIFF size/header wrong");
        Check(wave[4] == 75 && wave[20] == 0x66 && wave[21] == 1 && wave[22] == 2, "XMA format endianness changed");
        Check(wave[36] == 34 && wave[37] == 0 && wave[76] == 3 && wave[80] == 1 && wave[82] == 3, "WAV extension/payload wrong");
        session.package->rootChunks[0].data.pop_back();
        bool rejected = false;
        try { SelectedAudio(session); } catch (const std::exception&) { rejected = true; }
        Check(rejected, "Truncated audio accepted");
        session.package->rootChunks.clear();
        std::vector<uint8_t> cutHeader(76), cutData(23);
        std::copy(payload.begin() + 28, payload.begin() + 80, cutHeader.begin() + 24);
        Put32(cutData, 16, 3);
        cutData[20] = 1; cutData[21] = 2; cutData[22] = 3;
        session.package->rootChunks.push_back(CMChunk::Leaf(CutScene_Audio_SampleHeader, 1, cutHeader));
        session.package->rootChunks.push_back(CMChunk::Leaf(CutScene_Audio_SampleData, 1, cutData));
        session.selected = &session.package->rootChunks[1];
        Check(SelectedAudio(session).data == sample.data, "Cutscene audio resolution differs");
        std::filesystem::path temporary;
        { TemporaryDirectory files; temporary = files.path; WriteFile(files.path / "test", {1}); Check(std::filesystem::exists(temporary), "Temporary directory missing"); }
        Check(!std::filesystem::exists(temporary), "Temporary files leaked");
        std::cout << "XMA2 format, RIFF wrapper, cutscene and cleanup tests passed\n";
        return 0;
    }
    catch (const std::exception& ex) { std::cerr << ex.what() << '\n'; return 1; }
}
