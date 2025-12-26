/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#include "OffscreenFramebuffer.hpp"
#include <algorithm>
#include <iostream>

namespace
{
    int ValidateMSAASamples(int samples)
    {
        GLint max_samples = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

        samples = std::max(2, std::min(samples, max_samples));

        samples = samples & ~1;

        return samples;
    }
}

OffscreenFramebuffer::~OffscreenFramebuffer()
{
    Shutdown();
}

void OffscreenFramebuffer::Initialize(int width, int height, [[maybe_unused]] MSAA use_msaa, [[maybe_unused]] int msaa_samples)
{
    currentWidth  = width;
    currentHeight = height;

    useMSAA       = (use_msaa == MSAA::True);
    msaaSamples   = ValidateMSAASamples(msaa_samples);

    createResolveFramebuffer();
    createMSAAFramebuffer();
}

void OffscreenFramebuffer::BindForRendering()
{

    GLuint target = useMSAA ? msaaFramebuffer : resolveFramebuffer;
    // GLuint target = resolveFramebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, target);
}

GLuint OffscreenFramebuffer::GetTexture()
{
    resolveMSAA();
    return resolveTexture;
}

void OffscreenFramebuffer::Resize(int width, int height)
{
    currentWidth  = width;
    currentHeight = height;

    createResolveFramebuffer();
    createMSAAFramebuffer();
}

void OffscreenFramebuffer::SetMSAA([[maybe_unused]] MSAA use_msaa, [[maybe_unused]] int msaa_samples)
{
    useMSAA     = use_msaa == MSAA::False ? false : true;
    msaaSamples = ValidateMSAASamples(msaa_samples);

    if (currentWidth > 0 && currentHeight > 0)
    {
        createMSAAFramebuffer();
    }
}

void OffscreenFramebuffer::Shutdown()
{
    if (resolveFramebuffer != 0)
    {
        glDeleteFramebuffers(1, &resolveFramebuffer);
        resolveFramebuffer = 0;
    }
    if (resolveTexture != 0)
    {
        glDeleteTextures(1, &resolveTexture);
        resolveTexture = 0;
    }

    if (msaaFramebuffer != 0)
    {
        glDeleteFramebuffers(1, &msaaFramebuffer);
        msaaFramebuffer = 0;
    }
    if (msaaColorRenderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &msaaColorRenderbuffer);
        msaaColorRenderbuffer = 0;
    }
}

void OffscreenFramebuffer::createResolveFramebuffer()
{
    if (resolveFramebuffer == 0)
    {
        glGenFramebuffers(1, &resolveFramebuffer);
    }

    if (resolveTexture != 0)
    {
        glDeleteTextures(1, &resolveTexture);
        resolveTexture = 0;
    }

    glGenTextures(1, &resolveTexture);
    glBindTexture(GL_TEXTURE_2D, resolveTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#ifdef IS_WEBGL2
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, currentWidth, currentHeight);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, currentWidth, currentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, resolveFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Failed to create resolve framebuffer\n";
        std::exit(-1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffscreenFramebuffer::createMSAAFramebuffer()
{
    if (!useMSAA)
        return;

    if (msaaFramebuffer == 0)
    {
        glGenFramebuffers(1, &msaaFramebuffer);
    }

    if (msaaColorRenderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &msaaColorRenderbuffer);
        msaaColorRenderbuffer = 0;
    }

    glGenRenderbuffers(1, &msaaColorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRenderbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_RGBA8, currentWidth, currentHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRenderbuffer);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Failed to create MSAA framebuffer\n";
        std::exit(-1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffscreenFramebuffer::resolveMSAA()
{

    if (!useMSAA)
        return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer);
    glBlitFramebuffer(0, 0, currentWidth, currentHeight, 0, 0, currentWidth, currentHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
