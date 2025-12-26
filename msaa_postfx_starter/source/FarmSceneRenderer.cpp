/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#include "FarmSceneRenderer.hpp"
#include "Path.hpp"
#include <cmath>
#include <stb_perlin.h>

namespace SceneHelpers
{
    SDFTransform calculateSDFTransform(const mat3& transform, float line_width) noexcept
    {
        constexpr int  mat3_width = 3;
        constexpr auto mat3_index = [](int row, int col)
        {
            return col * mat3_width + row;
        };
        const auto  a = transform[mat3_index(0, 0)];
        const auto  b = transform[mat3_index(0, 1)];
        const auto  c = transform[mat3_index(1, 0)];
        const auto  d = transform[mat3_index(1, 1)];
        const vec2  world_size{ (std::sqrt(a * a + c * c)), (std::sqrt(b * b + d * d)) };
        const float line_width_addition = std::max((line_width), 0.0f);
        const vec2  quad_size           = { world_size[0] + line_width_addition, world_size[1] + line_width_addition };

        const vec2 scale_up       = { quad_size[0] / world_size[0], quad_size[1] / world_size[1] };
        mat3       quad_transform = transform;
        quad_transform[0] *= scale_up[0];
        quad_transform[1] *= scale_up[0];
        quad_transform[3] *= scale_up[1];
        quad_transform[4] *= scale_up[1];
        return { quad_transform, world_size, quad_size };
    }

    mat3 createTransformMatrix(float scale_x, float scale_y, float rotation_radians, float translate_x, float translate_y)
    {
        const float rad   = rotation_radians;
        const float cos_r = std::cos(rad);
        const float sin_r = std::sin(rad);

        return mat3{ scale_x * cos_r, scale_x * sin_r, 0.0f, -scale_y * sin_r, scale_y * cos_r, 0.0f, translate_x, translate_y, 1.0f };
    }

    mat3 mat3Multiply(const mat3& a, const mat3& b)
    {
        mat3 result;
        for (std::size_t col = 0; col < 3; ++col)
        {
            for (std::size_t row = 0; row < 3; ++row)
            {
                result[col * 3 + row] = a[0 * 3 + row] * b[col * 3 + 0] + a[1 * 3 + row] * b[col * 3 + 1] + a[2 * 3 + row] * b[col * 3 + 2];
            }
        }
        return result;
    }

    constexpr std::array<float, 4> hexToRGBA(unsigned int hex)
    {
        const unsigned int r = (hex >> 16) & 0xFF;
        const unsigned int g = (hex >> 8) & 0xFF;
        const unsigned int b = hex & 0xFF;
        return { static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, 1.0f };
    }

    void drawShape(
        const OpenGL::CompiledShader& shader, GLsizei indices_count, const mat3& to_ndc, int shape, float cx, float cy, float width, float height, const std::array<float, 4>& fill_color,
        const std::array<float, 4>& line_color, float line_width, float rotation)
    {
        const auto model         = createTransformMatrix(width, height, rotation, cx, cy);
        const auto sdf_transform = calculateSDFTransform(model, line_width);


        glUniformMatrix3fv(shader.UniformLocations.at("uToNDC"), 1, GL_FALSE, to_ndc.data());
        glUniformMatrix3fv(shader.UniformLocations.at("uModel"), 1, GL_FALSE, sdf_transform.QuadTransform.data());
        glUniform2f(shader.UniformLocations.at("uSDFScale"), sdf_transform.QuadSize[0], sdf_transform.QuadSize[1]);


        glUniform4fv(shader.UniformLocations.at("uFillColor"), 1, fill_color.data());
        glUniform4fv(shader.UniformLocations.at("uLineColor"), 1, line_color.data());
        glUniform2fv(shader.UniformLocations.at("uWorldSize"), 1, sdf_transform.WorldSize.data());
        glUniform1f(shader.UniformLocations.at("uLineWidth"), line_width);
        glUniform1i(shader.UniformLocations.at("uShape"), shape);

        glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_SHORT, nullptr);
    }
}

FarmSceneRenderer::~FarmSceneRenderer()
{
    Shutdown();
}

void FarmSceneRenderer::Initialize()
{
    const std::filesystem::path vertex_file   = assets::locate_asset("Assets/shaders/sdf.vert");
    const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/sdf.frag");
    shader                                    = OpenGL::CreateShader(vertex_file, fragment_file);

    struct Vertex
    {
        float x;
        float y;
    };

    const Vertex vertices[] = {
        { -0.5, -0.5 },
        { +0.5, -0.5 },
        { +0.5, +0.5 },
        { -0.5, +0.5 },
    };

    const unsigned short indices[] = {
        0, 1, 2, 0, 2, 3,
    };

    indicesCount = static_cast<GLsizei>(std::ssize(indices));


    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribDivisor(0, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FarmSceneRenderer::Render(int viewport_width, int viewport_height, float animation_time, float zoom)
{
    using namespace SceneHelpers;

    glUseProgram(shader.Shader);
    glBindVertexArray(vertexArrayObject);

    const mat3 to_ndc{ 2.0f / (static_cast<float>(viewport_width) * zoom), 0.0f, 0.0f, 0.0f, 2.0f / (static_cast<float>(viewport_height) * zoom), 0.0f, -1.0f / zoom, -1.0f / zoom, 1.0f };


    constexpr float reference_width  = 800.0f;
    constexpr float reference_height = 600.0f;
    const float     cam_x            = (reference_width - static_cast<float>(viewport_width)) * 0.5f - 0.589f;
    const float     cam_y            = (reference_height - static_cast<float>(viewport_height)) * 0.5f + 0.235f;


    constexpr float right_x = 1.0f;
    constexpr float right_y = 0.0f;
    constexpr float up_x    = 0.0f;
    constexpr float up_y    = 1.0f;
    const float     view_tx = -(right_x * cam_x + right_y * cam_y);
    const float     view_ty = -(up_x * cam_x + up_y * cam_y);

    const mat3 view{ right_x, up_x, 0.0f, right_y, up_y, 0.0f, view_tx, view_ty, 1.0f };

    const mat3 view_ndc = mat3Multiply(to_ndc, view);

    const auto    transparent = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f };
    constexpr int CIRCLE      = 0;
    constexpr int RECT        = 1;


    const float sky_width    = static_cast<float>(viewport_width + 2);
    const float sky_height   = static_cast<float>(std::max(viewport_height, 300)) - 200.0f + 2;
    const float sky_center_x = 400;
    const float sky_center_y = 200.0f + sky_height * 0.5f;
    drawShape(shader, indicesCount, view_ndc, RECT, sky_center_x, sky_center_y, sky_width, sky_height, hexToRGBA(0x87CEEB), transparent, 0.0f);


    const float ground_width    = static_cast<float>(viewport_width) + 2;
    const float ground_height   = static_cast<float>(std::max(viewport_height, 300)) - 400.0f + 2;
    const float ground_center_x = 400;
    const float ground_center_y = 200.0f - ground_height * 0.5f;
    drawShape(shader, indicesCount, view_ndc, RECT, ground_center_x, ground_center_y, ground_width, ground_height, hexToRGBA(0x90EE90), transparent, 0.0f);


    const float ground_top_y    = ground_center_y + ground_height * 0.5f;
    const float ground_bottom_y = ground_center_y - ground_height * 0.5f;
    const float ground_left_x   = ground_center_x - ground_width * 0.5f;
    const float ground_right_x  = ground_center_x + ground_width * 0.5f;

    const float sway_animation_s = std::sin(animation_time * 1.5f * 0.25f);
    const float sway_animation_c = std::cos(animation_time * 1.5f * 0.25f);


    for (float y = ground_bottom_y + 10.0f; y < ground_top_y; y += 15.0f)
    {
        for (float x = ground_left_x + 10.0f; x < ground_right_x; x += 15.0f)
        {
            const float noise_value = stb_perlin_noise3(x * 0.02f, y * 0.02f, 0.0f, 0, 0, 0);
            if (noise_value < 0.3f)
                continue;


            if (y > 150 && ((x > 100 && x < 280) || (x > 300 && x < 400) || (x > 500 && x < 650)))
                continue;

            const float phase      = noise_value * 3.14159f;
            const float grass_sway = (sway_animation_s * std::cos(phase) + sway_animation_c * std::sin(phase)) * 0.2f;

            const float blade_noise = stb_perlin_noise3(x * 0.05f, y * 0.05f, 1.0f, 0, 0, 0);
            const int   blade_count = 3 + static_cast<int>((blade_noise + 1.0f) * 1.0f);

            for (int i = 0; i < blade_count; ++i)
            {
                const float offset_x     = static_cast<float>(i) * 3.0f - static_cast<float>(blade_count - 1) * 1.5f;
                const float height_noise = stb_perlin_noise3(x * 0.1f + static_cast<float>(i), y * 0.1f, 2.0f, 0, 0, 0);
                const float blade_height = 10.0f + height_noise * 3.0f;
                const float blade_sway   = grass_sway + std::sin(static_cast<float>(i) * 1.5f) * 0.03f;

                drawShape(shader, indicesCount, view_ndc, RECT, x + offset_x, y, 1, blade_height, hexToRGBA(0x228B22), transparent, 0.0f, blade_sway);
            }
        }
    }


    drawShape(shader, indicesCount, view_ndc, CIRCLE, 700, 520, 100, 100, hexToRGBA(0xFFD700), hexToRGBA(0xFFA500), 3.0f);


    const float cloud_speed     = 20.0f;
    const float cloud_width     = 150.0f;
    const float screen_width    = static_cast<float>(viewport_width);
    const float wrap_distance   = screen_width + cloud_width;
    const float cloud_offset    = std::fmod(animation_time * cloud_speed, wrap_distance);
    const float left_screen_pad = -cloud_width * 0.5f;


    const float cloud1_base = 150.0f - cam_x;
    const float cloud1_x    = std::fmod(cloud1_base + cloud_offset + wrap_distance, wrap_distance) + left_screen_pad + cam_x;
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud1_x, 500, 60, 60, hexToRGBA(0xFFFFFF), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud1_x + 30, 500, 70, 70, hexToRGBA(0xFFFFFF), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud1_x + 60, 500, 60, 60, hexToRGBA(0xFFFFFF), transparent, 0.0f);


    const float cloud2_base = 450.0f - cam_x;
    const float cloud2_x    = std::fmod(cloud2_base + cloud_offset + wrap_distance, wrap_distance) + left_screen_pad + cam_x;
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud2_x, 450, 50, 50, hexToRGBA(0xFFFFFF), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud2_x + 25, 450, 60, 60, hexToRGBA(0xFFFFFF), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, cloud2_x + 50, 450, 50, 50, hexToRGBA(0xFFFFFF), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 175, 240, 150, 120, hexToRGBA(0xD2691E), hexToRGBA(0x8B4513), 3.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 175, 315, 190, 50, hexToRGBA(0x8B0000), hexToRGBA(0x5A0000), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 205, 215, 50, 70, hexToRGBA(0x654321), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 220, 215, 8, 8, hexToRGBA(0xFFD700), hexToRGBA(0x000000), 1.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 140, 260, 40, 40, hexToRGBA(0x87CEEB), hexToRGBA(0x000000), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 140, 260, 4, 40, hexToRGBA(0x000000), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 140, 260, 40, 4, hexToRGBA(0x000000), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 575, 240, 150, 120, hexToRGBA(0x4682B4), hexToRGBA(0x27408B), 3.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 575, 315, 190, 50, hexToRGBA(0x2F4F4F), hexToRGBA(0x1C1C1C), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 605, 215, 50, 70, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 620, 215, 8, 8, hexToRGBA(0xFFD700), hexToRGBA(0x000000), 1.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 540, 260, 40, 40, hexToRGBA(0x87CEEB), hexToRGBA(0x000000), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 540, 260, 4, 40, hexToRGBA(0x000000), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 540, 260, 40, 4, hexToRGBA(0x000000), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 365, 235, 30, 70, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 365 + sway_animation_s * 5.0f, 290, 80, 80, hexToRGBA(0x228B22), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 345 + sway_animation_s * 4.0f, 280, 70, 70, hexToRGBA(0x228B22), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 385 + sway_animation_s * 6.0f, 280, 70, 70, hexToRGBA(0x228B22), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 742.5, 230, 25, 60, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 742 + sway_animation_s * 4.0f, 275, 70, 70, hexToRGBA(0x32CD32), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 725 + sway_animation_s * 3.5f, 265, 60, 60, hexToRGBA(0x32CD32), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 760 + sway_animation_s * 5.0f, 265, 60, 60, hexToRGBA(0x32CD32), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 330, 115, 100, 50, hexToRGBA(0x228B22), hexToRGBA(0x006400), 3.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 350, 150, 60, 40, hexToRGBA(0x228B22), hexToRGBA(0x006400), 3.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 350, 149.5, 40, 25, hexToRGBA(0x87CEEB), hexToRGBA(0x000000), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 270, 107.5, 40, 35, hexToRGBA(0xFFD700), hexToRGBA(0xFFA500), 2.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 329, 177.5, 8, 25, hexToRGBA(0x696969), hexToRGBA(0x000000), 1.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 350, 100, 60, 60, hexToRGBA(0x2F4F4F), hexToRGBA(0x000000), 4.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 350, 100, 30, 30, hexToRGBA(0x696969), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 270, 90, 40, 40, hexToRGBA(0x2F4F4F), hexToRGBA(0x000000), 3.0f);
    drawShape(shader, indicesCount, view_ndc, CIRCLE, 270, 90, 20, 20, hexToRGBA(0x696969), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 55, 195, 10, 50, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 95, 195, 10, 50, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 455, 195, 10, 50, hexToRGBA(0x8B4513), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 495, 195, 10, 50, hexToRGBA(0x8B4513), transparent, 0.0f);


    drawShape(shader, indicesCount, view_ndc, RECT, 75, 202, 60, 6, hexToRGBA(0xA0522D), transparent, 0.0f);
    drawShape(shader, indicesCount, view_ndc, RECT, 475, 202, 60, 6, hexToRGBA(0xA0522D), transparent, 0.0f);


    const auto draw_flower = [&](float base_x, float base_y, float stem_width, float stem_length, float head_radius, unsigned int head_color)
    {
        const float stem_angle = sway_animation_c * 0.2f;
        const float cosa       = std::cos(stem_angle);
        const float sina       = std::sin(stem_angle);
        const float stem_top_x = -sina * 0.5f * stem_length;
        const float stem_top_y = cosa * 0.5f * stem_length;
        drawShape(shader, indicesCount, view_ndc, RECT, base_x, base_y, stem_width, stem_length, hexToRGBA(0x228B22), transparent, 0.0f, stem_angle);
        drawShape(shader, indicesCount, view_ndc, CIRCLE, base_x + stem_top_x, base_y + stem_top_y, head_radius, head_radius, hexToRGBA(head_color), transparent, 0.0f);
    };


    draw_flower(182, 67.5, 4, 25, 16, 0xFF69B4);
    draw_flower(222, 70, 4, 30, 16, 0xFF1493);
    draw_flower(602, 65, 4, 20, 14, 0xFFD700);
    draw_flower(642, 68.5, 4, 27, 16, 0xFF6347);

    glBindVertexArray(0);
    glUseProgram(0);
}

void FarmSceneRenderer::Shutdown()
{
    if (vertexBuffer != 0)
    {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (indexBuffer != 0)
    {
        glDeleteBuffers(1, &indexBuffer);
        indexBuffer = 0;
    }
    if (vertexArrayObject != 0)
    {
        glDeleteVertexArrays(1, &vertexArrayObject);
        vertexArrayObject = 0;
    }
    if (shader.Shader != 0)
    {
        glDeleteProgram(shader.Shader);
        shader.Shader = 0;
    }
}
