/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


#include "ImGuiHelper.hpp"

#include <SDL.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace
{
    std::string   gImGuiSaveFilePath;
    SDL_Window*   gCachedWindow    = nullptr;
    SDL_GLContext gCachedGLContext = nullptr;
}

namespace ImGuiHelper
{
    void Initialize(gsl::not_null<SDL_Window*> sdl_window, gsl::not_null<SDL_GLContext> gl_context, const std::filesystem::path& ini_file_path)
    {
        gCachedWindow    = sdl_window;
        gCachedGLContext = gl_context;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        {
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            gImGuiSaveFilePath = ini_file_path.string();
            io.IniFilename     = gImGuiSaveFilePath.c_str();
        }
        ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
        ImGui_ImplOpenGL3_Init();
    }

    void FeedEvent(const SDL_Event& event)
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    Viewport Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();


        {
            const ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
            const ImGuiWindowFlags   window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            const ImGuiViewport* const main_viewport   = ImGui::GetMainViewport();
            const auto                 window_position = main_viewport->Pos;
            const auto                 window_size     = main_viewport->Size;
            ImGui::SetNextWindowPos(window_position);
            ImGui::SetNextWindowSize(window_size);
            ImGui::SetNextWindowViewport(main_viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Main Window Dockspace", nullptr, window_flags);
            ImGui::PopStyleVar(3);
            const ImGuiID dockspace_id = ImGui::GetID("Main Window Dockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);


            const ImGuiDockNode* const node = ImGui::DockBuilderGetCentralNode(dockspace_id);
            if (node == nullptr) [[unlikely]]
                return Viewport{ 0, 0, static_cast<int>(window_size.x), static_cast<int>(window_size.y) };

            const int opengl_x = static_cast<int>((node->Pos.x - window_position.x));
            const int opengl_y = static_cast<int>((window_size.y - ((node->Pos.y - window_position.y) + node->Size.y)));
            const int opengl_w = static_cast<int>(node->Size.x);
            const int opengl_h = static_cast<int>(node->Size.y);
            ImGui::End();
            return Viewport{ opengl_x, opengl_y, opengl_w, opengl_h };
        }
    }

    void End()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        const ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(gCachedWindow, gCachedGLContext);
        }
    }

    void Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        gCachedWindow    = nullptr;
        gCachedGLContext = nullptr;
    }
}
