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

class OffscreenFramebuffer
{
public:
    OffscreenFramebuffer() = default;
    ~OffscreenFramebuffer();


    OffscreenFramebuffer(const OffscreenFramebuffer&)            = delete;
    OffscreenFramebuffer& operator=(const OffscreenFramebuffer&) = delete;

    enum class MSAA : bool
    {
        False,
        True
    };
    void   Initialize(int width, int height, MSAA use_msaa = MSAA::False, int msaa_samples = 4);
    void   BindForRendering();
    GLuint GetTexture();
    void   Resize(int width, int height);
    void   SetMSAA(MSAA use_msaa, int msaa_samples);

    int GetMSAASamples() const
    {
        return msaaSamples;
        // return 0;
    }

    void Shutdown();

private:
    int            currentWidth{ 0 };
    int            currentHeight{ 0 };

    bool           useMSAA{ false };
    int            msaaSamples{ 4 };
    OpenGL::Handle resolveFramebuffer{ 0 };
    OpenGL::Handle resolveTexture{ 0 };

    OpenGL::Handle msaaFramebuffer{ 0 };
    OpenGL::Handle msaaColorRenderbuffer{ 0 };

    void createResolveFramebuffer();
    void createMSAAFramebuffer();
    void resolveMSAA();
};
