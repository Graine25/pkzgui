#pragma once
#include "package/package_session.h"
#include "model/model_geometry.h"
#include "shader/shader_program.h"
#include <future>
#include <vector>
#include <string>
#include <cmath>

struct ModelPreview
{
    ModelPreview() = default;
    ModelPreview(const ModelPreview&) = delete;
    ModelPreview& operator=(const ModelPreview&) = delete;
    ~ModelPreview();

    void Update(const PackageSession& session);
    void Draw(PackageSession* session = nullptr);
    void DrawActions(PackageSession& session);
    std::vector<std::string> TakeLogMessages();
    void SetTextured(bool enabled) { textured = enabled; renderDirty = true; }
    void LoadPNG(size_t material, const std::string& path);
    void UsePackageTexture(size_t material);
    void UseRetexture(size_t material);
    void ClearRetexture(size_t material);
    void CompileGamePass(size_t material, size_t pass);
    void LoadShaderPreview(size_t material, const std::string& path);
    void ApplyPNG(PackageSession& session, size_t material);
    const std::vector<ModelMesh>& Meshes() const { return meshes; }
    const std::vector<ModelMaterial>& Materials() const { return materials; }
    void Clear();

    bool active = false;
    std::string name, error;
    size_t meshCount = 0, vertexCount = 0, triangleCount = 0;

private:
    const CMChunk* selected = nullptr;
    const PKPackage* package = nullptr;
    uint64_t revision = 0;

    std::vector<ModelMesh> meshes;
    std::vector<ModelMaterial> materials;
    std::vector<std::unique_ptr<ShaderProgram>> materialShaders;
    struct SamplerTexture { unsigned int texture=0; uint32_t format=0, target=0; bool attempted=false; };
    std::vector<std::vector<SamplerTexture>> shaderTextures;
    std::future<ShaderAsset> shaderJob;
    std::string shaderJobPath, shaderJobLabel;
    size_t shaderJobMaterial=0;
    const CMChunk* shaderJobSelected=nullptr;
    const PKPackage* shaderJobPackage=nullptr;
    uint64_t shaderJobRevision=0;
    int shaderPassIndex=0;
    void PollShaderJob(const PackageSession& session);
    void DrawShaderActions();
    void ReloadShader(size_t material);
    unsigned int ShaderTextureID(size_t material, size_t sampler, unsigned target);
    unsigned int UploadCube(const std::array<std::vector<uint8_t>,6>& faces, uint32_t size);
    void InstallShader(size_t material, const ShaderAsset& asset);

    struct MaterialTexture {
        unsigned int texture = 0, retexture = 0;
        bool attempted = false;
        size_t bytes = 0, retextureBytes = 0;
        bool useRetexture = false;
        uint32_t pngCRC = 0;
        std::string png, error;
    };
    std::vector<MaterialTexture> materialTextures;
    size_t textureBytes = 0;
    bool textured = false, twoSided = true, hudCollapsed = false;
    int materialIndex = 0;
    float uvDivisor = 1024.f;
    std::vector<std::string> logMessages;
    bool DrawHUD(float x, float y, float width, float height);
    void SelectMaterial(const char* id);
    void Log(const std::string& message);
    unsigned int MaterialTextureID(size_t index);
    unsigned int UploadTexture(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);
    struct GPUMesh { unsigned int vao = 0, positions = 0, normals = 0, indices = 0, texcoords = 0, tangents = 0, colors = 0; };
    std::vector<GPUMesh> gpuMeshes;
    bool renderDirty = true;
    unsigned int program = 0;
    unsigned int fbo = 0, colorTex = 0, depthRbo = 0, whiteTexture = 0, whiteCube = 0;
    int fboW = 0, fboH = 0;

    // Orbit camera
    float yaw = 0.6f, pitch = 0.35f, distance = 3.0f;
    float centerX = 0, centerY = 0, centerZ = 0;
    float bboxMin[3]{}, bboxMax[3]{};
    bool hasBBox = false;

    void EnsureProgram();
    void EnsureFBO(int w, int h);
    void DestroyGL();
    bool DecodeGeometry(const PackageSession& session);
    void ComputeBounds();
    void UploadMeshes();
    void RenderScene(int w, int h);
};
