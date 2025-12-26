/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#pragma once

#include "OffscreenFramebuffer.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct PostProcessingEffect
{
    std::string Name;
    enum class Enable : bool
    {
        False,
        True
    };
    Enable                                Enabled;
    OpenGL::CompiledShader                Shader;
    std::unique_ptr<OffscreenFramebuffer> Framebuffer;

    using SetUniformsFunction = std::function<void(const OpenGL::CompiledShader&)>;
    SetUniformsFunction SetUniforms;

    PostProcessingEffect(
        const std::string& name, Enable enabled, OpenGL::CompiledShader shader, SetUniformsFunction set_uniforms = [](const OpenGL::CompiledShader&) { })
        : Name(name), Enabled(enabled), Shader(shader), Framebuffer(nullptr), SetUniforms(set_uniforms)
    {
    }
};

class PostProcessingPipeline
{
public:
    PostProcessingPipeline() = default;
    ~PostProcessingPipeline();

    PostProcessingPipeline(const PostProcessingPipeline&)            = delete;
    PostProcessingPipeline& operator=(const PostProcessingPipeline&) = delete;

    void                  Initialize(int width, int height);
    void                  AddEffect(PostProcessingEffect&& effect);
    GLuint                Apply(GLuint input_texture);
    void                  Resize(int width, int height);
    void                  Shutdown();
    PostProcessingEffect* GetEffect(const std::string& name);

private:
    std::vector<PostProcessingEffect> effects{};
    int                               currentWidth{ 0 };
    int                               currentHeight{ 0 };
    OpenGL::Handle                    fullscreenVAO{ 0 };
    OpenGL::Handle                    fullscreenVBO{ 0 };
    GLsizei                           fullscreenVertexCount{ 0 };

    void createFramebuffer(PostProcessingEffect& effect);
    void renderEffect(const PostProcessingEffect& effect, GLuint input_texture);
    void setupFullscreenTriangle();
};
