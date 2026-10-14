#pragma once
#include "package/package_session.h"
#include <vector>
#include <string>
#include <cmath>

struct ModelMesh
{
    std::vector<float> positions;   // xyz
    std::vector<float> normals;     // xyz
    std::vector<uint32_t> indices;
    uint32_t material = 0;
};

struct ModelPreview
{
    ModelPreview() = default;
    ModelPreview(const ModelPreview&) = delete;
    ModelPreview& operator=(const ModelPreview&) = delete;
    ~ModelPreview();

    void Update(const PackageSession& session);
    void Draw();
    void Clear();

    bool active = false;
    std::string name, error;
    size_t meshCount = 0, vertexCount = 0, triangleCount = 0;

private:
    const CMChunk* selected = nullptr;
    const PKPackage* package = nullptr;
    uint64_t revision = 0;

    std::vector<ModelMesh> meshes;
    unsigned int program = 0;
    unsigned int fbo = 0, colorTex = 0, depthRbo = 0;
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
    void RenderScene(int w, int h);
};
