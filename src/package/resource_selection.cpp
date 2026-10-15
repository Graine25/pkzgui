#include "package/resource_selection.h"

namespace
{
    const CMChunk* FindHeader(const CMChunk& node, const CMChunk* selected, const CMChunk* header = nullptr)
    {
        if (node.GetMaskedID() == GenSub_Resource || node.GetMaskedID() == Gen_PostLoadData)
            header = node.FindChild(GenSub_ResourceHeader);
        if (&node == selected)
            return header;
        for (const CMChunk& child : node.children)
            if (const CMChunk* found = FindHeader(child, selected, header))
                return found;
        return nullptr;
    }
}

std::optional<CMChunkResourceHeader> SelectedResource(const PKPackage& package, const CMChunk* selected)
{
    if (!selected)
        return std::nullopt;
    for (const CMChunk& root : package.rootChunks)
        if (const CMChunk* header = FindHeader(root, selected))
            return CMChunkResourceHeader(*header);
    return std::nullopt;
}
