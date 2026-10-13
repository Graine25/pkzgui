#include "package/package_session.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

void FinishLoad(PackageSession& session)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (session.IsLoading())
    {
        if (std::chrono::steady_clock::now() > deadline)
            throw std::runtime_error("Background loading timed out");
        session.PollLoad();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    try
    {
        Check(argc == 2, "Expected fixture directory");
        const std::filesystem::path directory = std::filesystem::u8path(argv[1]);
        std::filesystem::create_directories(directory);
        PackageSession session;
        for (bool littleEndian : {false, true})
        {
            PKPackage fixture;
            fixture.isLittleEndian = littleEndian;
            CMChunk root = CMChunk::Container(CMChunkTypes::Root, 1);
            root.AddChild(CMChunk::Leaf(CMChunkTypes::Gen_LevelHeader, 3, {1, 2, 3, 4}));
            fixture.rootChunks.push_back(std::move(root));
            for (bool compressed : {false, true})
            {
                const auto path = directory / std::filesystem::u8path("package-\xC3\xA9.pkz");
                std::vector<uint8_t> bytes = fixture.Serialize();
                if (compressed)
                    bytes = BUCompress::Compress(bytes);
                std::ofstream output(path, std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                output.close();
                Check(session.Load(path.u8string()), "Fixture should load");
                Check(session.error.empty(), "Successful load should clear errors");
                Check(session.package->isLittleEndian == littleEndian, "Wrong byte order");
                Check(session.package->wasCompressed == compressed, "Wrong compression state");
                Check(session.selected == &session.package->rootChunks.front(), "Initial selection missing");
                Check(session.selected->children.front().data == std::vector<uint8_t>({1, 2, 3, 4}), "Payload changed");
            }
        }
        const PKPackage* previous = session.package.get();
        const CMChunk* previousSelection = session.selected;
        const auto invalid = directory / "invalid.pak";
        std::ofstream(invalid, std::ios::binary).put('x');
        Check(!session.Load(invalid.u8string()), "Invalid package should fail");
        Check(!session.error.empty(), "Load failure should explain the error");
        Check(session.package.get() == previous && session.selected == previousSelection, "Failed load discarded previous package");
        Check(!session.Load((directory / "missing.pkz").u8string()), "Missing file should fail");
        PKPackage emptyLeaf;
        emptyLeaf.isLittleEndian = true;
        emptyLeaf.rootChunks.push_back(CMChunk::Leaf(CMChunkTypes::Root, 1, {}));
        const auto leafPath = directory / "empty-leaf.pak";
        emptyLeaf.WriteToFile(leafPath.u8string());
        Check(session.Load(leafPath.u8string()) && session.package->isLittleEndian, "Empty leaf byte order detection failed");
        Check(session.BeginLoad(leafPath.u8string()) && session.IsLoading(), "Loading state missing");
        Check(!session.BeginLoad(invalid.u8string()), "Overlapping load should be rejected");
        FinishLoad(session);
        Check(session.package && session.error.empty(), "Background load failed");
        previous = session.package.get();
        Check(session.BeginLoad(invalid.u8string()), "Failed to start background load");
        FinishLoad(session);
        Check(session.package.get() == previous && !session.error.empty(), "Background failure discarded previous package");
        std::cout << "Package loading checks passed\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
