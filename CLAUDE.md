# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is a cross-platform OpenGL project using CMake with presets for different configurations.

### Linux (Current Platform)
```bash
# Configure for debug build
cmake --preset linux-debug

# Build debug version
cmake --build --preset linux-debug

# Configure for release build
cmake --preset linux-release

# Build release version
cmake --build --preset linux-release

# For web builds (requires Emscripten setup)
cmake --preset web-debug
cmake --build --preset web-debug
```

### Windows
```bash
# Configure for debug build
cmake --preset windows-debug

# Build debug version
cmake --build --preset windows-debug

# For web builds on Windows
cmake --preset web-debug-on-windows
cmake --build --preset web-debug-on-windows
```

## Development Environment

The project requires specific development tools:
- **C++20** compiler support
- **CMake 3.21+**
- **Ninja build system** (for Unix-like systems)
- **OpenGL**, **SDL2**, **GLEW**, **ImGui** libraries

For web development, **Emscripten 4.0.13** is required. See `docs/DevEnvironment.md` for complete setup instructions.

## Project Architecture

### Core Structure
- **main.cpp**: Entry point with SDL/OpenGL initialization and main game loop
- **Shader.hpp/cpp**: OpenGL shader compilation and management utilities
- **Handle.hpp**: RAII wrapper for OpenGL resource management
- **ImGuiHelper.hpp/cpp**: Dear ImGui integration for debug UI
- **Random.hpp/cpp**: Random number generation utilities

### Graphics Pipeline
The project implements a 2D graphics system using:
- **Vertex/Fragment shaders** with matrix transformations (model-to-NDC pipeline)
- **Vertex Array Objects (VAOs)** for geometry organization
- **Buffer management** for vertices and indices
- **Matrix-based transformations** (scale, rotation, translation)

### Multi-Platform Support
- **Native desktop**: Direct OpenGL context
- **Web (Emscripten)**: WebGL 2.0 with automatic HTML shell generation
- **Platform-specific optimizations**: Different build configurations for debug/release

### Key Dependencies
All dependencies are managed via CMake FetchContent:
- **OpenGL**: Graphics API
- **SDL2**: Window management and input handling
- **GLEW**: OpenGL extension loading
- **Dear ImGui**: Immediate mode GUI
- **GSL**: Microsoft Guidelines Support Library

## Development Notes

The project uses a component-based architecture with global state management. The main rendering loop handles:
1. Event processing (SDL events + ImGui)
2. Game logic updates with time-based animations
3. OpenGL rendering with shader-based transformations
4. ImGui debug interface rendering

Web builds generate single-file HTML output with embedded assets, suitable for direct browser deployment.