/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#pragma once

#include "Shader.hpp"
#include <GL/glew.h>
#include <array>

using mat3 = std::array<float, 9>;
using vec2 = std::array<float, 2>;

struct SDFTransform
{
    mat3 QuadTransform;
    vec2 WorldSize;
    vec2 QuadSize;
};

namespace SceneHelpers
{

    SDFTransform                   calculateSDFTransform(const mat3& transform, float line_width) noexcept;
    mat3                           createTransformMatrix(float scale_x, float scale_y, float rotation_radians, float translate_x, float translate_y);
    mat3                           mat3Multiply(const mat3& a, const mat3& b);
    constexpr std::array<float, 4> hexToRGBA(unsigned int hex);
    void                           drawShape(
                                  const OpenGL::CompiledShader& shader, GLsizei indices_count, const mat3& to_ndc, int shape, float cx, float cy, float width, float height, const std::array<float, 4>& fill_color,
                                  const std::array<float, 4>& line_color, float line_width, float rotation = 0.0f);
}

class FarmSceneRenderer
{
public:
    FarmSceneRenderer() = default;
    ~FarmSceneRenderer();

    FarmSceneRenderer(const FarmSceneRenderer&)            = delete;
    FarmSceneRenderer& operator=(const FarmSceneRenderer&) = delete;

    void Initialize();
    void Render(int viewport_width, int viewport_height, float animation_time, float zoom);
    void Shutdown();

private:
    OpenGL::CompiledShader shader{};
    OpenGL::Handle         vertexBuffer{ 0 };
    OpenGL::Handle         indexBuffer{ 0 };
    OpenGL::Handle         vertexArrayObject{ 0 };
    GLsizei                indicesCount{ 0 };
};
