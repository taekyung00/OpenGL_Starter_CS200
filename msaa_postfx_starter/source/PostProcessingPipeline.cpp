/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#include "PostProcessingPipeline.hpp"
#include <array>
#include <iostream>

PostProcessingPipeline::~PostProcessingPipeline()
{
    Shutdown();
}

void PostProcessingPipeline::Initialize(int width, int height)
{
    currentWidth  = width;
    currentHeight = height;

    setupFullscreenTriangle();

    for (auto& effect : effects)
    {
        createFramebuffer(effect);
    }
}

void PostProcessingPipeline::AddEffect(PostProcessingEffect&& effect)
{
    effects.push_back(std::move(effect));

    if (currentWidth > 0 && currentHeight > 0)
    {
        createFramebuffer(effects.back());
    }
}

GLuint PostProcessingPipeline::Apply(GLuint input_texture)
{
    GLuint current_texture = input_texture;

    for (const auto& effect : effects)
    {
        if (effect.Enabled == PostProcessingEffect::Enable::True && effect.Framebuffer)
        {
            renderEffect(effect, current_texture);
            current_texture = effect.Framebuffer->GetTexture();
        }
    }

    return current_texture;
}

void PostProcessingPipeline::Resize(int width, int height)
{
    currentWidth  = width;
    currentHeight = height;

    for (auto& effect : effects)
    {
        if (effect.Framebuffer)
        {
            effect.Framebuffer->Resize(currentWidth, currentHeight);
        }
    }
}

void PostProcessingPipeline::Shutdown()
{
    for (auto& effect : effects)
    {
        if (effect.Framebuffer)
        {
            effect.Framebuffer->Shutdown();
            effect.Framebuffer.reset();
        }
        if (effect.Shader.Shader != 0)
        {
            glDeleteProgram(effect.Shader.Shader);
            effect.Shader.Shader = 0;
        }
    }
    effects.clear();

    if (fullscreenVAO != 0)
    {
        glDeleteVertexArrays(1, &fullscreenVAO);
        fullscreenVAO = 0;
    }
    if (fullscreenVBO != 0)
    {
        glDeleteBuffers(1, &fullscreenVBO);
        fullscreenVBO = 0;
    }
}

PostProcessingEffect* PostProcessingPipeline::GetEffect(const std::string& name)
{
    for (auto& effect : effects)
    {
        if (effect.Name == name)
        {
            return &effect;
        }
    }
    return nullptr;
}

void PostProcessingPipeline::createFramebuffer(PostProcessingEffect& effect)
{
    if (!effect.Framebuffer)
    {
        effect.Framebuffer = std::make_unique<OffscreenFramebuffer>();
    }

    effect.Framebuffer->Initialize(currentWidth, currentHeight, OffscreenFramebuffer::MSAA::False);
}

void PostProcessingPipeline::renderEffect(const PostProcessingEffect& effect, GLuint input_texture)
{
    effect.Framebuffer->BindForRendering();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, currentWidth, currentHeight);

    glUseProgram(effect.Shader.Shader);

    effect.SetUniforms(effect.Shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture);
    glUniform1i(effect.Shader.UniformLocations.at("uColorTexture"), 0);

    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, fullscreenVertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessingPipeline::setupFullscreenTriangle()
{
    struct FullscreenVertex
    {
        float x, y;
        float u, v;
    };

    const FullscreenVertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 0.0f },
        {  3.0f, -1.0f, 2.0f, 0.0f },
        { -1.0f,  3.0f, 0.0f, 2.0f },
    };

    fullscreenVertexCount = static_cast<GLsizei>(std::ssize(vertices));

    glGenBuffers(1, &fullscreenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &fullscreenVAO);
    glBindVertexArray(fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FullscreenVertex), nullptr);
    glVertexAttribDivisor(0, 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FullscreenVertex), reinterpret_cast<void*>(2 * sizeof(float)));
    glVertexAttribDivisor(1, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
