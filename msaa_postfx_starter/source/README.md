# Source Start - Simple Farm Scene

This folder contains a simplified starting point for the Post-Processing FX project.

## What's Included

This version includes:
- **Farm Scene Renderer**: Draws the animated farm scene with SDF shapes
- **Basic Offscreen Framebuffer**: Simple non-multisampled framebuffer
- **Simple Texture Shader**: Uses `simple-texture.frag` to display the framebuffer
- **No Post-Processing**: No post-processing pipeline or effects
- **No MSAA**: No multisampling support

## Purpose

This is designed as a **starting point for live coding demonstrations** where you can:

1. **Add MSAA Support**: Incrementally add multisampling to the `OffscreenFramebuffer` class
   - Add MSAA framebuffer and renderbuffer
   - Add resolve functionality
   - Add UI controls for MSAA settings

2. **Add Post-Processing Effects**: Build the post-processing pipeline step by step
   - Create `PostProcessingPipeline` class
   - Add individual effects (Box Blur, Gamma Correction, Chromatic Aberration)
   - Chain effects together
   - Add UI controls for each effect

## Key Differences from Full Version

### OffscreenFramebuffer
**Simplified:**
- Single framebuffer with texture attachment
- `Initialize(width, height)` - no MSAA parameters
- `GetTexture()` - direct texture access
- No resolve step needed

**Full Version:**
- Dual framebuffer system (MSAA + resolve)
- `Initialize(width, height, use_msaa, msaa_samples)`
- `ResolveAndGetTexture()` - blits MSAA to resolve buffer
- `SetMSAA()` to toggle MSAA at runtime

### demo.cpp
**Simplified:**
- Renders scene to offscreen buffer
- Displays buffer directly to screen using `simple-texture.frag`
- Basic ImGui controls (zoom, animation)
- ~180 lines of code

**Full Version:**
- Adds MSAA settings UI
- Adds post-processing pipeline
- Adds effect-specific UI controls (blur, gamma, chromatic aberration)
- Shows effect output textures
- ~400 lines of code

## Building

To build with this starting point instead of the full source:

1. Modify the root `CMakeLists.txt` to use `source_start` instead of `source`:
   ```cmake
   # Change this line:
   add_subdirectory(source)
   # To:
   add_subdirectory(source_start)
   ```

2. Reconfigure and build your project as normal.

## Live Coding Steps

Suggested order for live coding demonstration:

### Step 1: Add MSAA to Framebuffer
1. Add MSAA framebuffer and renderbuffer members
2. Add MSAA parameters to `Initialize()`
3. Implement `createMSAAFramebuffer()`
4. Implement `resolveMSAA()` and `ResolveAndGetTexture()`
5. Update `BindForRendering()` to choose correct target

### Step 2: Add MSAA UI Controls
1. Add MSAA enable checkbox
2. Add MSAA sample count dropdown
3. Wire up `SetMSAA()` to update framebuffer

### Step 3: Create Post-Processing Pipeline
1. Create `PostProcessingPipeline.hpp` and `.cpp`
2. Implement fullscreen quad rendering
3. Implement framebuffer creation for effects
4. Implement effect chaining in `Apply()`

### Step 4: Add Post-Processing Effects
1. Add Box Blur effect with UI controls
2. Add Gamma Correction effect with UI controls
3. Add Chromatic Aberration effect with UI controls
4. Add effect output texture visualization

## Files in This Folder

- `demo.cpp` - Simplified main demo code
- `OffscreenFramebuffer.hpp/cpp` - Basic framebuffer (no MSAA)
- `FarmSceneRenderer.hpp/cpp` - Farm scene renderer (unchanged)
- `main.cpp` - Main entry point (unchanged)
- `Shader.hpp/cpp` - Shader utilities (unchanged)
- `Path.hpp/cpp` - Asset path utilities (unchanged)
- `ImGuiHelper.hpp/cpp` - ImGui integration (unchanged)
- `Handle.hpp` - OpenGL handle type alias (unchanged)
- `CMakeLists.txt` - Build configuration (PostProcessingPipeline removed)
