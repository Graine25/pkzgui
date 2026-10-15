# Model previews and shader reuse

## Opening and replacing a texture

1. Open `BaseGameplay.pkz`, expand `Gen_GeometryLibrary`, and select a named geometry resource such as `SpiderManAmazing_Casual[Hi]`.
2. Enable **Package textures** in the bottom-right overlay on the model. Textures resolve through the material's sampler CRC, not filename guesses. Disable it for a solid preview.
3. Choose a material in the overlay or in **Model exports / textures**, below Fields on the right. `OfficeSuit` uses `SMA_PPCasual_D[Hi]`; Skin, Eye and Hair share the head texture.
4. **Extract package PNG...** saves the original image for editing. **Preview PNG...** changes that material's preview without editing the package. **Use package texture** discards the preview override and uses the package's current texture.
5. **Apply PNG to package** replaces the referenced texture using PKZLib's existing encoder, its original dimensions, mip layout and compression format. This affects every material sharing that texture. Use **File → Save As** to write an edited copy.

A PNG can be previewed even when the referenced texture lives in another package. Applying it requires the texture to be in the open package. A preview override does not add a new texture resource or change a material's sampler reference. Selecting another model or editing/reloading the package clears preview overrides.

The overlay contains **Two-sided**, **Frame model** and an **Adjustments...** popup for the UV divisor and per-material **Cutout alpha**. Operation results and diagnostic errors appear in the Debug log below the model.

**Cutout alpha** is optional per material. Diffuse alpha can encode other information: applying an alpha test indiscriminately removes the casual character's face. Exact game alpha state is not inferred here. **Two-sided** and **Frame model** help inspect the geometry.

This is a static geometry and base-color preview. It does not run the game's material shaders, compose scene hierarchies, animate bones/morphs, or reproduce its lighting, normal/specular maps, decals, animated UVs and effects. Unsupported layouts produce an error instead of substituting unrelated geometry.

## Asset loading

The CPU loader lives in `src/model/model_geometry.*` and `model_material.*`; OpenGL preview resources live in `model_preview.*`.

- Resource identity includes CRC, data offset, language and quality. Geometry buffers come from the resource header's post-load offset. The lowest numbered available LOD is paired with the same LOD index in post-load data.
- Mesh offsets, counts, stride and material indices come from `Geo_GeomMesh` v8. Material sampler/stage data and pass declarations come from `Geo_MaterialInfo` v64, as described by PKZLib's `schemas/EOT-360.xml`.
- Stream-zero position, normal and base-texture UV attributes use the embedded D3D declaration. Supported preview inputs are float3 positions, packed 11:11:10 or float3 normals, and signed/unsigned short2 or float2 UVs. Packed positions and other layouts fail explicitly.
- Triangle strips follow the draw core documented at `GRRender3dObj_DrawSubmeshInstanced` / `0x82129F60` in reeot's research. Degenerates and restart markers are handled without deleting triangles based on edge length.
- Integer UVs use a divisor of 1024, confirmed in OfficeSuit's recompiled vertex shader: constant `c255.x = asfloat(0x3A800000)` multiplies both coordinates. The divisor can be adjusted for another shader's encoding; float/normalized UVs are independent of it.
- Geometry and texture uploads are cached per selection. The framebuffer is redrawn on changes or camera movement. Geometry and texture caches each have a 128 MiB budget; texture dimensions are capped at 4096 per side. Rendering uses at most a 1920×1080 framebuffer.

Local verification used the user's BaseGameplay package: 126 geometry resources loaded and rendered without a loader/render exception; 328 materials' embedded shader containers passed structural validation. Casual has eight meshes, 7,474 vertices and 10,929 triangles. This is a useful coverage check, not proof that every effect looks identical to the game. Game data and generated shader binaries remain outside version control.

## Reusing reeot-XenosRecomp

**Export material shaders...** writes a `.bin` containing the material's original VS/PS shader containers and a `.bin.json` sidecar with pass slot/set, sampler CRCs, material flags and vertex declarations. The container is the virtual shader data followed by physical microcode, excluding the runtime object header (`0x368` bytes for VS, `0x28` for PS). Empty/referenced-only shader records export no container.

The sidecar records each pass's byte ranges. Hashing a range with XXH3-64 yields the lookup identity used by reeot's shader cache; serialized shader IDs are not substitutes for this hash. Repeated containers are allowed and XenosRecomp deduplicates them.

With the reeot fork built, convert the export using its matching `shader_common.h`:

```sh
REEOT_ROOT=../reeot-dni
/path/to/XenosRecomp OfficeSuit.bin OfficeSuit-hlsl \
  "$REEOT_ROOT/thirdparty/XenosRecomp/XenosRecomp/shader_common.h" --hlsl
```

OfficeSuit was tested through the local fork on macOS arm64: 84 embedded containers became 38 unique HLSL shaders, with zero conversion failures. DXC also compiled all 38 to nonempty SPIR-V cache entries. Its emitted UV scaling confirms the preview's divisor.

The fork's directory-input mode builds a compiled cache. Put the exported `.bin` in a dedicated input directory and pass a C++ output filename:

```sh
/path/to/XenosRecomp shader-input OfficeSuit-cache.cpp \
  "$REEOT_ROOT/thirdparty/XenosRecomp/XenosRecomp/shader_common.h"
```

The cache contains proprietary game shader data and is a local artifact, not a source dependency to commit or bundle.

The HLSL/compiled cache is not connected to the current OpenGL renderer. Adding an import button alone cannot provide the game's shading. The next implementation should:

1. Keep geometry/material parsing independent of the renderer. Add a guest-shader backend that consumes reeot's output; keep the current base-color renderer as a fallback while that backend matures.
2. Reuse the fork's compiled SPIR-V and its vertex-fetch/interpolant metadata, or compile its HLSL using the matching `REEOT_RECOMP` contract. Preserve pass variants and select compatible VS/PS interpolants. Match the fork's exact cache revision: its SPIR-V payloads are compressed with smol-v and the outer cache uses zstd.
3. Reproduce the binding contract from `src/gpu/shaders/guest_shaders.*`, `shader_cache.h`, `vertex_layout.cpp`, `format.cpp` and XenosRecomp's `shader_common.h`: vertex/pixel float-register files, shared constants, descriptor indices, booleans, specialization masks and the packed-normal/signed-UV corrections. Raw buffers and bindless descriptor access prevent directly dropping these shaders into the present OpenGL 3.x programs.
4. Start with one captured OfficeSuit draw, its camera/world constants and its actual material pass. Render it offscreen, compare against reeot, then add normal/specular textures, lighting and skin/morph variants. Cache by container hash and specialization state; perform conversion outside the frame loop.

The export sidecar describes package bindings; it is not a complete runtime draw capture. Shader containers do not contain live camera/bone matrices, scene lighting, render targets, all dynamically selected states, or runtime resource pointers.

## Where default.xex research would help

Further executable reversing is not required for the static textured preview. A single known-good draw captured from reeot is the most useful next input: container hashes, pass slot/set, vertex declaration/streams, VS and PS float constants, shared constants/spec mask, sampler CRCs, alpha/cull/blend state and the resulting image.

If those values cannot be obtained from the port, trace these paths in the matching executable:

- `RZGeometry::Load` (`0x8216CB28`), `RZGeometry::ResolveCRC` (`0x8216D358`), `LoadLodHeader` (`0x8217AD30`), `LoadSubmeshBuffers` (`0x8217B0C8`) and `LoadIndexData` (`0x8217B558`) for unhandled geometry variants and palette/morph streams.
- `GRRender3dObj_SetInstanceWorldAndBones` (`0x82129548`) for the palette-to-matrix mapping and bind/animated pose constants.
- Material pass application and sampler/constant setup around `0x82129700` and `0x82129318`, followed by the draw core at `0x82129F60`, for exact alpha/blend and shader constant construction.

Addresses and structure interpretations come from the local reeot-dni research, particularly `87_the_pipeline_set.md`, `RE_03_render_feed.toml`, `RE_20_engine_symbols.toml` and `tools/pso/pso_paks.py`. The old `RE_07_materials.toml` has a correction identifying an earlier material/environment mix-up; its old outer-record interpretation should not drive a new loader.
