#pragma once
#include "package/package_session.h"
#include "Resource/RZTexture.h"

RZTexture SelectedTexture(const PackageSession& session);
void ExtractTexture(const PackageSession& session, const std::string& path);
void ReplaceTexture(PackageSession& session, const std::string& path, uint32_t width, uint32_t height);

void ReplaceTexture(PackageSession& session, const CMChunkResourceHeader& header, const std::string& path, uint32_t width, uint32_t height);

void ExtractTexture(const PackageSession& session, const CMChunkResourceHeader& header, const std::string& path);
