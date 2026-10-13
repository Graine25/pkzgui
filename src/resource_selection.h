#pragma once
#include "Resource/ResourceHeader.h"
#include "Package/PKPackage.h"
#include <optional>

std::optional<CMChunkResourceHeader> SelectedResource(const PKPackage& package, const CMChunk* selected);
