# GSX shader previews

GSX can convert an embedded Xbox 360 material pass with reeot-XenosRecomp, compile its HLSL through DXC/SPIR-V, and run the resulting GLSL in the model viewer. The existing base-color preview remains available per material. These replacements affect GSX previews only; they do not change Xbox shader bytecode in the package.

## Build

```sh
git submodule update --init --recursive
cmake -S . -B build -DGSX_SHADER_COMPILER=ON
cmake --build build -j
./build/bin/pkzgui
```

Conversion is optional and defaults to OFF. A build without it can still load a previously converted `.gsxshader` file, but cannot convert game shaders or reload HLSL edits. The converter pins the reeot fork alongside its matching `shader_common.h`; no separate reeot checkout or shader-header path is needed at runtime. DXC's shared library is copied beside the executable and included when installing a build with conversion enabled. The bundled DXC supports macOS arm64, Windows x64/arm64 and Linux x64; Linux arm64 needs a compatible DXC build supplied to the dependency.

## BaseGameplay example

1. Open `BaseGameplay.pkz`, expand `Gen_GeometryLibrary`, and select `SpiderManAmazing_Casual[Hi]`.
2. In **Model exports / textures**, select **OfficeSuit** and expand **Shader previews**.
3. Select **Technique 0 / static**, then **Preview game shaders**. Conversion runs in the background; progress and failures appear in the Debug log. It installs only if the selected model and package revision still match.
4. Toggle **Use game shader preview** to compare with the base-color renderer. The model overlay's **Textures** toggle switches all textured/shader previews to solid geometry and back.
5. The material's original sampler CRCs resolve its package textures, including 2D normal/specular inputs and cube reflection maps. **Package texture / Retexture** still selects the base texture used by this shader. Loading a PNG previews it; applying it to the package remains a separate action.
6. **Save editable shader...** writes a `.gsxshader` asset and two companions, `.gsxshader.vert.hlsl` and `.gsxshader.frag.hlsl`. Edit the companions and press **Reload HLSL edits**. Keep the generated entry point, vertex semantics and constant/texture binding contract. Reflection names/registers come from the original pass; changing that contract requires an updated asset/converter.
7. A failed compile/link keeps the last working preview. Save again after a successful edit to update the `.gsxshader` asset. **Load shader preview...** loads that saved result on another compatible material without recompiling it.

Local DLC verification also decoded all four `_DLC001.pak` cube maps and converted, linked and drew Prodigy static slots 0–4 and 6–10. Slot 5 has no embedded pixel stage and is not selectable as a VS/PS preview. A slot-0 check across all 23 DLC materials converted, linked and drew successfully. Ricochet’s `SMA_State_00` was also checked through asynchronous package loading and the GUI shader-job path. Its separate TEXCOORD1 coordinates are decoded and uploaded from the mesh declaration. This is a compatibility check, not confirmation of each slot’s game rendering purpose.

The OfficeSuit static pass was converted from the local BaseGameplay package and rendered on macOS arm64. This verifies the conversion/render path; it does not establish exact game lighting or support for every material pass. Game packages, converted assets and shader sources are local derived data and are not committed.

## Constants and game appearance

Studio defaults provide the viewer camera, identity world transform, material color, simple ambient lighting, decoded vertex attributes and package texture-sign flags. Unrecognized registers default to zero. The guest shader executes its original math, including its own UV scaling; the base renderer's UV divisor does not override that math.

Shaders alone do not contain the live world/bone matrices, lights, boolean registers, render targets or blend/depth/cull decisions used by a game draw. Some effects can therefore look wrong under studio defaults. Shader preview currently uses the viewer's opaque draw state and two-sided setting; it does not reconstruct a multipass scene.

**Load game constants...** accepts one 9,216-byte little-endian binary snapshot in this order:

| Byte range | Data |
| --- | --- |
| 0–4095 | VS float4 registers, 256 entries |
| 4096–8191 | PS float4 registers, 256 entries |
| 8192–9215 | Shared uint4 registers, 64 entries, matching the pinned `REEOT_RECOMP` contract |

By default the viewer replaces reflected view/projection registers with its own camera. **Use captured camera** preserves those registers too. **Use studio constants** resets the snapshot. CPU vertex decoding already handles endianness, integer UVs and packed normals, so the adapter clears the corresponding shader correction flags. It keeps supported alpha-test specialization and disables unsupported screen-space specialization. Other runtime resources/states are not included in this snapshot.

The most useful next input is one known-good OfficeSuit draw captured from reeot: pass slot/set, VS/PS container hashes, these constants, sampler bindings, alpha/cull/blend state and a reference image. That is more immediately useful than reversing more of `default.xex`. See [model-preview.md](model-preview.md#where-defaultxex-research-would-help) for executable entry points if capture is unavailable.

## Current scope

- Static, stream-zero position/normal/tangent/color and TEXCOORD0–3 inputs. Two-component signed/unsigned short and float UV channels retain their own offsets and values. The base-color renderer still uses the material’s selected UV set; guest shaders receive the individual channels they declare. Missing channels fail explicitly. Skin/morph/instance streams remain future work.
- Embedded VS/PS pairs with compatible interpolants. Missing/referenced-only shaders require their source package.
- Up to 16 active 2D/cube texture bindings. Static texture heap accesses are mapped to separate, typed OpenGL units, avoiding sampler-type conflicts and unused 16-slot arrays. Cube maps use their own header and decode all six base faces; currently supported layouts store equally sized face chains with tiled mip surfaces. Volume/1D textures and dynamic texture indexing remain unsupported. Unresolved package textures use a matching white fallback and produce a Debug log entry. No render-target/history textures or scene passes are recreated.
- GLSL 150/OpenGL uniform buffers, with cached geometry/textures/programs and redraw only when the preview changes. Conversion and HLSL compilation run outside the frame loop; GPU linking/install occurs on the GUI thread.
- Geometry and texture uploads retain the existing memory budgets. Shader source fields are bounded to 4 MiB, saved assets to 20 MiB, and register metadata to the 256-register files.

Xbox-compatible shader replacement would need a separate compiler/serializer path. Host HLSL or GLSL is never written into the PKZ's Xbox shader containers.

## Debug logs

The Debug log is saved automatically, including shader failures and the active material/slot/set. **Copy log** copies the visible session messages. Hover the Debug log heading to see the saved file path.

- macOS: `~/Library/Logs/GSX/gsx.log`
- Windows: `%LOCALAPPDATA%/GSX/logs/gsx.log`
- Linux: `$XDG_STATE_HOME/gsx/gsx.log`, falling back to `~/.local/state/gsx/gsx.log`

Startup records the executable path, build date/time and whether shader conversion is enabled. The log appends across runs and rotates to `gsx.previous.log` after 2 MiB. Saved logs include local asset paths and diagnostics.
