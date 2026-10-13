#include "audio_player.h"
#include "audio_process.h"
#include "file_io.h"
#include "miniaudio.h"
#include <future>
#include <chrono>

struct AudioPlayer::State
{
    struct Result
    {
        std::shared_ptr<TemporaryDirectory> files;
        std::filesystem::path wav;
        std::string message;
        bool play = false;
    };
    std::future<Result> pending;
    std::shared_ptr<TemporaryDirectory> playingFiles;
    ma_engine engine{};
    ma_sound sound{};
    bool initialized = false, hasSound = false;
};

AudioPlayer::AudioPlayer() : state(std::make_unique<State>()) {}
AudioPlayer::~AudioPlayer()
{
    Stop();
    if (state->initialized) ma_engine_uninit(&state->engine);
}
bool AudioPlayer::IsBusy() const { return state->pending.valid(); }
void AudioPlayer::Stop()
{
    if (state->hasSound) { ma_sound_uninit(&state->sound); state->hasSound = false; }
    state->playingFiles.reset();
}
void AudioPlayer::Begin(AudioSample sample, const std::string& exportPath)
{
    if (IsBusy()) return;
    if (exportPath.empty()) Stop();
    state->pending = std::async(std::launch::async, [sample = std::move(sample), exportPath] {
        State::Result result;
        result.files = std::make_shared<TemporaryDirectory>();
        const auto input = result.files->path / "xma.wav";
        result.wav = result.files->path / "decoded.wav";
        WriteFile(input, BuildXmaContainer(sample.format, sample.data.data(), sample.data.size()));
        RunFfmpeg(input, result.wav);
        result.play = exportPath.empty();
        const std::string format = std::to_string(sample.format.samplesPerSec) + " Hz, " + std::to_string(sample.format.channels) + " ch";
        if (result.play) result.message = "Playing " + sample.name + " (" + format + ")";
        else
        {
            std::filesystem::copy_file(result.wav, std::filesystem::u8path(exportPath), std::filesystem::copy_options::overwrite_existing);
            result.message = "Extracted " + exportPath + " (" + format + ")";
        }
        return result;
    });
}
void AudioPlayer::Poll(std::string& status, std::string& error)
{
    if (state->hasSound && !ma_sound_is_playing(&state->sound)) { Stop(); status = "Playback finished."; }
    if (!IsBusy() || state->pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try
    {
        auto result = state->pending.get();
        if (result.play)
        {
            if (!state->initialized)
            {
                if (ma_engine_init(nullptr, &state->engine) != MA_SUCCESS) throw std::runtime_error("Cannot open audio playback device.");
                state->initialized = true;
            }
            Stop();
            if (ma_sound_init_from_file(&state->engine, result.wav.u8string().c_str(), 0, nullptr, nullptr, &state->sound) != MA_SUCCESS)
                throw std::runtime_error("Cannot load decoded WAV for playback.");
            state->hasSound = true;
            state->playingFiles = std::move(result.files);
            if (ma_sound_start(&state->sound) != MA_SUCCESS) { Stop(); throw std::runtime_error("Cannot start audio playback."); }
        }
        status = result.message;
    }
    catch (const std::exception& ex) { error = ex.what(); status = "Audio operation failed."; }
}
