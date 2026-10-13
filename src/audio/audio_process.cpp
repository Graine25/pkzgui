#include "audio/audio_process.h"
#include <random>
#include <stdexcept>
#include <chrono>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <cerrno>
#include <unistd.h>
extern char** environ;
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

TemporaryDirectory::TemporaryDirectory()
{
    std::random_device random;
    for (int i = 0; i < 32; ++i)
    {
        path = std::filesystem::temp_directory_path() / ("pkzgui-" + std::to_string(random()) + "-" + std::to_string(random()));
        if (std::filesystem::create_directory(path)) return;
    }
    throw std::runtime_error("Cannot create temporary audio directory.");
}
TemporaryDirectory::~TemporaryDirectory()
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
}

std::filesystem::path FindFfmpeg()
{
    std::filesystem::path executable;
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size && size < buffer.size()) executable = std::wstring(buffer.data(), size);
    const char* name = "ffmpeg.exe";
#else
    const char* name = "ffmpeg";
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) executable = buffer.data();
#else
    std::vector<char> buffer(32768);
    const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (size > 0 && static_cast<size_t>(size) < buffer.size()) executable = std::string(buffer.data(), size);
#endif
#endif
    if (!executable.empty() && std::filesystem::is_regular_file(executable.parent_path() / name))
        return executable.parent_path() / name;
    if (std::filesystem::is_regular_file(name)) return std::filesystem::absolute(name);
#ifdef PKZGUI_FFMPEG_PATH
    if (std::filesystem::is_regular_file(PKZGUI_FFMPEG_PATH)) return PKZGUI_FFMPEG_PATH;
#endif
    throw std::runtime_error("FFmpeg not found beside pkzgui. Build external/FFmpeg and rebuild pkzgui; see README.md.");
}

void RunFfmpeg(const std::filesystem::path& input, const std::filesystem::path& output)
{
    const auto program = FindFfmpeg();
#ifdef _WIN32
    const auto quote = [](const std::wstring& value) {
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (wchar_t c : value)
        {
            if (c == L'\\') { ++slashes; continue; }
            result.append(c == L'\"' ? slashes * 2 + 1 : slashes, L'\\');
            slashes = 0;
            result += c;
        }
        result.append(slashes * 2, L'\\');
        return result + L'\"';
    };
    std::wstring command = quote(program.native()) + L" -hide_banner -loglevel error -nostdin -y -i " + quote(input.native()) + L" -c:a pcm_s16le " + quote(output.native());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        throw std::runtime_error("Failed to launch FFmpeg.");
    const DWORD waited = WaitForSingleObject(process.hProcess, 60000);
    if (waited != WAIT_OBJECT_0) { TerminateProcess(process.hProcess, 1); WaitForSingleObject(process.hProcess, INFINITE); }
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (waited != WAIT_OBJECT_0 || code) throw std::runtime_error("FFmpeg failed to decode XMA2 or timed out.");
#else
    std::vector<std::string> arguments = {program.string(), "-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-i", input.string(), "-c:a", "pcm_s16le", output.string()};
    std::vector<char*> argv;
    for (auto& argument : arguments) argv.push_back(argument.data());
    argv.push_back(nullptr);
    pid_t process = 0;
    if (posix_spawn(&process, program.c_str(), nullptr, nullptr, argv.data(), environ))
        throw std::runtime_error("Failed to launch FFmpeg.");
    int status = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    for (;;)
    {
        const pid_t result = waitpid(process, &status, WNOHANG);
        if (result == process) break;
        if (result < 0 && errno != EINTR) throw std::runtime_error("Failed waiting for FFmpeg.");
        if (std::chrono::steady_clock::now() >= deadline)
        {
            kill(process, SIGKILL);
            while (waitpid(process, &status, 0) < 0 && errno == EINTR) {}
            throw std::runtime_error("FFmpeg decoding timed out.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status)) throw std::runtime_error("FFmpeg failed to decode XMA2.");
#endif
}
