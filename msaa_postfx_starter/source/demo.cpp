/**
 * \file
 * \author Rudy Castan
 * \author Taekyung Ho
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#include "FarmSceneRenderer.hpp"
#include "OffscreenFramebuffer.hpp"
#include "Path.hpp"
#include "PostProcessingPipeline.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array>
#include <imgui.h>
#include <iostream>
#include <string>
#include <vector>

extern int gWidth;
extern int gHeight;


FarmSceneRenderer      gFarmScene;
float                  gZoom = 1.0f;
OffscreenFramebuffer   gOffscreenBuffer;
PostProcessingPipeline gPostProcessing;


OpenGL::CompiledShader gScreenShader;
OpenGL::Handle         gScreenVAO;
OpenGL::Handle         gScreenVBO;
GLsizei                gScreenVertexCount = 0;


float gAnimationTime  = 0.0f;
bool  gPauseAnimation = false;


bool  gEnablePostFX              = true;
bool  gUseMSAA                   = true;
int   gMSAASamples               = 4;
float gBoxBlurSize               = 2.0f;
float gBoxBlurSeparation         = 1.0f;
float gGammaValue                = 2.2f;
float gChromaticAberrationMouseX = 0.5f;
float gChromaticAberrationMouseY = 0.5f;
int   gPixelSize                 = 5;

void setupScreenTriangle()
{
    struct ScreenVertex
    {
        float x, y;
        float u, v;
    };

    const ScreenVertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 0.0f },
        {  3.0f, -1.0f, 2.0f, 0.0f },
        { -1.0f,  3.0f, 0.0f, 2.0f },
    };

    gScreenVertexCount = static_cast<GLsizei>(std::ssize(vertices));

    glGenBuffers(1, &gScreenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &gScreenVAO);
    glBindVertexArray(gScreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gScreenVBO);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), nullptr);
    glVertexAttribDivisor(0, 0);


    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), reinterpret_cast<void*>(2 * sizeof(float)));
    glVertexAttribDivisor(1, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void demo_setup()
{
    gFarmScene.Initialize();

    const auto use_msaa = gUseMSAA ? OffscreenFramebuffer::MSAA::True : OffscreenFramebuffer::MSAA::False;
    gOffscreenBuffer.Initialize(gWidth, gHeight, use_msaa, gMSAASamples);


    const std::filesystem::path screen_vert = assets::locate_asset("Assets/shaders/simple.vert");
    const std::filesystem::path screen_frag = assets::locate_asset("Assets/shaders/simple-texture.frag");
    gScreenShader                           = OpenGL::CreateShader(screen_vert, screen_frag);

    setupScreenTriangle();


    gPostProcessing.Initialize(gWidth, gHeight);

    {
        const std::filesystem::path box_blur_vert   = assets::locate_asset("Assets/shaders/simple.vert");
        const std::filesystem::path box_blur_frag   = assets::locate_asset("Assets/shaders/box-blur.frag");
        auto                        box_blur_shader = OpenGL::CreateShader(box_blur_vert, box_blur_frag);

        gPostProcessing.AddEffect(PostProcessingEffect(
            "Box Blur", PostProcessingEffect::Enable::False, box_blur_shader,
            [](const OpenGL::CompiledShader& shader)
            {
                glUniform1i(shader.UniformLocations.at("uBlurSize"), static_cast<int>(gBoxBlurSize));
                glUniform1f(shader.UniformLocations.at("uSeparation"), gBoxBlurSeparation);
            }));
    }

    //{
    //    const std::filesystem::path gamma_vert   = assets::locate_asset("Assets/shaders/simple.vert");
    //    const std::filesystem::path gamma_frag   = assets::locate_asset("Assets/shaders/gamma-correct.frag");
    //    auto                        gamma_shader = OpenGL::CreateShader(gamma_vert, gamma_frag);

    //    gPostProcessing.AddEffect(PostProcessingEffect(
    //        "Gamma Correction", PostProcessingEffect::Enable::True, gamma_shader, [](const OpenGL::CompiledShader& shader) { glUniform1f(shader.UniformLocations.at("uGamma"), gGammaValue); }));
    //}

    //{
    //    const std::filesystem::path chroma_vert   = assets::locate_asset("Assets/shaders/simple.vert");
    //    const std::filesystem::path chroma_frag   = assets::locate_asset("Assets/shaders/chromatic-aberration.frag");
    //    auto                        chroma_shader = OpenGL::CreateShader(chroma_vert, chroma_frag);

    //    gPostProcessing.AddEffect(PostProcessingEffect(
    //        "Chromatic Aberration", PostProcessingEffect::Enable::True, chroma_shader,
    //        [](const OpenGL::CompiledShader& shader) { glUniform2f(shader.UniformLocations.at("uMouseFocusPoint"), gChromaticAberrationMouseX, gChromaticAberrationMouseY); }));
    //}
    //{
    //    const std::filesystem::path pixel_vert   = assets::locate_asset("Assets/shaders/simple.vert");
    //    const std::filesystem::path pixel_frag   = assets::locate_asset("Assets/shaders/pixelize.frag");
    //    auto                        pixel_shader = OpenGL::CreateShader(pixel_vert, pixel_frag);

    //    gPostProcessing.AddEffect(PostProcessingEffect(
    //        "Pixelization", PostProcessingEffect::Enable::True, pixel_shader, [](const OpenGL::CompiledShader& shader) { glUniform1i(shader.UniformLocations.at("pixelSize"), gPixelSize); })); // must be odd
    //}

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (gUseMSAA)
    {
        glEnable(GL_MULTISAMPLE);
    }
}

void demo_shutdown()
{
    gFarmScene.Shutdown();
    gOffscreenBuffer.Shutdown();
    gPostProcessing.Shutdown();

    if (gScreenVAO != 0)
    {
        glDeleteVertexArrays(1, &gScreenVAO);
        gScreenVAO = 0;
    }
    if (gScreenVBO != 0)
    {
        glDeleteBuffers(1, &gScreenVBO);
        gScreenVBO = 0;
    }
    if (gScreenShader.Shader != 0)
    {
        glDeleteProgram(gScreenShader.Shader);
        gScreenShader.Shader = 0;
    }
}

void demo_draw()
{
    static int last_width  = 0;
    static int last_height = 0;
    if (gWidth != last_width || gHeight != last_height)
    {
        gOffscreenBuffer.Resize(gWidth, gHeight);
        gPostProcessing.Resize(gWidth, gHeight);
        last_width  = gWidth;
        last_height = gHeight;
    }


    if (!gPauseAnimation)
    {
        gAnimationTime += 0.016f;
    }


    gOffscreenBuffer.BindForRendering();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, gWidth, gHeight);

    gFarmScene.Render(gWidth, gHeight, gAnimationTime, gZoom);


    GLuint scene_texture = gOffscreenBuffer.GetTexture();


    GLuint final_texture = scene_texture;
    if (gEnablePostFX)
    {
        final_texture = gPostProcessing.Apply(scene_texture);
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, gWidth, gHeight);

    glUseProgram(gScreenShader.Shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, final_texture);
    glUniform1i(gScreenShader.UniformLocations.at("uColorTexture"), 0);

    glBindVertexArray(gScreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, gScreenVertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void demo_imgui()
{
    ImGui::Begin("Farm Scene");

    float percent = 100.0f / gZoom;
    ImGui::SliderFloat("Zoom", &percent, 100.0f, 400.0f);
    gZoom = 100.0f / percent;

    ImGui::SeparatorText("Animation");
    ImGui::Checkbox("Pause Animation", &gPauseAnimation);

    ImGui::SeparatorText("MSAA Settings");
    bool msaa_changed = ImGui::Checkbox("Enable MSAA", &gUseMSAA);

    ImGui::BeginDisabled(!gUseMSAA);


    static GLint max_msaa_samples = 0;
    if (max_msaa_samples == 0)
    {
        glGetIntegerv(GL_MAX_SAMPLES, &max_msaa_samples);
    }


    static std::vector<int> valid_samples;
    if (valid_samples.empty())
    {
        for (int samples = 2; samples <= max_msaa_samples; samples *= 2)
        {
            valid_samples.push_back(samples);
        }
    }


    int current_index = 0;
    for (size_t i = 0; i < valid_samples.size(); ++i)
    {
        if (valid_samples[i] == gMSAASamples)
        {
            current_index = static_cast<int>(i);
            break;
        }
    }


    std::string preview = std::to_string(gMSAASamples) + "x";
    if (ImGui::BeginCombo("MSAA Samples", preview.c_str()))
    {
        for (size_t i = 0; i < valid_samples.size(); ++i)
        {
            bool        is_selected = (current_index == static_cast<int>(i));
            std::string label       = std::to_string(valid_samples[i]) + "x";

            if (ImGui::Selectable(label.c_str(), is_selected))
            {
                gMSAASamples = valid_samples[i];
                msaa_changed = true;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::EndDisabled();

    if (msaa_changed)
    {
        const auto use_msaa = gUseMSAA ? OffscreenFramebuffer::MSAA::True : OffscreenFramebuffer::MSAA::False;
        gOffscreenBuffer.SetMSAA(use_msaa, gMSAASamples);

        gMSAASamples = gOffscreenBuffer.GetMSAASamples();

        if (gUseMSAA)
        {
            glEnable(GL_MULTISAMPLE);
        }
        else
        {
            glDisable(GL_MULTISAMPLE);
        }
    }

    ImGui::SeparatorText("Post-Processing Effects");
    ImGui::Checkbox("Enable Post-FX", &gEnablePostFX);

    ImGui::BeginDisabled(!gEnablePostFX);


    if (auto* box_blur = gPostProcessing.GetEffect("Box Blur"))
    {
        ImGui::Checkbox(box_blur->Name.c_str(), reinterpret_cast<bool*>(&box_blur->Enabled));

        ImGui::BeginDisabled(box_blur->Enabled == PostProcessingEffect::Enable::False);
        ImGui::Indent();
        ImGui::SliderFloat("Blur Size", &gBoxBlurSize, 0.0f, 10.0f);
        ImGui::SliderFloat("Blur Separation", &gBoxBlurSeparation, 1.0f, 5.0f);
        ImGui::Unindent();
        ImGui::EndDisabled();
    }


    if (auto* gamma = gPostProcessing.GetEffect("Gamma Correction"))
    {
        ImGui::Checkbox(gamma->Name.c_str(), reinterpret_cast<bool*>(&gamma->Enabled));

        ImGui::BeginDisabled(gamma->Enabled == PostProcessingEffect::Enable::False);
        ImGui::Indent();
        ImGui::SliderFloat("Gamma", &gGammaValue, 0.5f, 4.0f);
        ImGui::Unindent();
        ImGui::EndDisabled();
    }


    if (auto* chroma = gPostProcessing.GetEffect("Chromatic Aberration"))
    {
        ImGui::Checkbox(chroma->Name.c_str(), reinterpret_cast<bool*>(&chroma->Enabled));

        ImGui::BeginDisabled(chroma->Enabled == PostProcessingEffect::Enable::False);
        ImGui::Indent();
        ImGui::SliderFloat("Focus X", &gChromaticAberrationMouseX, 0.0f, 1.0f);
        ImGui::SliderFloat("Focus Y", &gChromaticAberrationMouseY, 0.0f, 1.0f);
        ImGui::Unindent();
        ImGui::EndDisabled();
    }

    if (auto* pixelization = gPostProcessing.GetEffect("Pixelization"))
    {
        ImGui::Checkbox(pixelization->Name.c_str(), reinterpret_cast<bool*>(&pixelization->Enabled));

        ImGui::BeginDisabled(pixelization->Enabled == PostProcessingEffect::Enable::False);
        ImGui::SliderInt("Pixel Size", &gPixelSize, 1, 800);
        if ((gPixelSize & 1) == 0)
        {
            gPixelSize += 1; // make sure it's odd
        }
        // ImGui::Indent();
        // ImGui::Unindent();
        ImGui::EndDisabled();
    }

    ImGui::EndDisabled();

    ImGui::SeparatorText("Render Textures");


    static int         selected_effect_index = -1;
    static const char* effect_names[]        = { "Box Blur", "Gamma Correction", "Chromatic Aberration", "Pixelization" };
    static const char* current_effect_name   = "None";

    if (selected_effect_index >= 0 && selected_effect_index < 4)
    {
        current_effect_name = effect_names[selected_effect_index];
    }

    if (ImGui::BeginCombo("View Effect", current_effect_name))
    {
        if (ImGui::Selectable("None", selected_effect_index == -1))
        {
            selected_effect_index = -1;
        }

        for (int i = 0; i < 4; i++)
        {
            bool is_selected = (selected_effect_index == i);
            if (ImGui::Selectable(effect_names[i], is_selected))
            {
                selected_effect_index = i;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }


    if (gEnablePostFX && selected_effect_index >= 0 && selected_effect_index < 4)
    {
        if (auto* effect = gPostProcessing.GetEffect(effect_names[selected_effect_index]))
        {
            if (effect->Enabled == PostProcessingEffect::Enable::True && effect->Framebuffer)
            {
                ImGui::Text("Output Texture (%s):", effect->Name.c_str());
                GLuint texture = effect->Framebuffer->GetTexture();

                const float     aspect_ratio   = static_cast<float>(gWidth) / static_cast<float>(gHeight);
                constexpr float display_width  = 400.0f;
                const float     display_height = display_width / aspect_ratio;

                ImGui::Image(static_cast<ImTextureRef>(texture), ImVec2(display_width, display_height), ImVec2(0, 1), ImVec2(1, 0));
            }
            else if (effect->Enabled == PostProcessingEffect::Enable::False)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Effect is disabled");
            }
        }
    }

    ImGui::End();
}
