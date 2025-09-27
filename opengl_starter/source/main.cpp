/**
 * \file
 * \author Rudy Castan
 * \date 2024 Fall
 * \copyright DigiPen Institute of Technology
 */


#include "ImGuiHelper.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <gsl/gsl> //owner template
#include <iostream>

gsl::owner<SDL_Window*> gWindow = nullptr; // mental reminder
gsl::owner<SDL_GLContext> gContext = nullptr;
bool                      gIsDone  = false;
int                       gWidth        = 800;
int                       gHeight       = 600;

#if defined(__EMSCRIPTEN__)
#    include <emscripten.h> //for emscripten_set_main_loop

//./app_resources/web check html it assume we have setWindowSize
#    include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(main_window)
{
    // from js to c++
    emscripten::function("setWindowSize", emscripten::optional_override([](int sizeX, int sizeY) { SDL_SetWindowSize(gWindow, sizeX, sizeY); }));
}

#endif

void demo_setup();
void demo_draw();
void demo_imgui();


void setup()
{
    ImGuiHelper::Initialize(gWindow, gContext);
    demo_setup();
}

void main_loop();

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef DEVELOPER_VERSION
    std::cout << "Developer Version\n";
#endif

#ifdef IS_WEBGL2
    std::cout << "WebGL 2 Version\n";
#else
    std::cout << "OpenGL Desktop Version\n";
#endif

    std::cout << "Hello World!\nRudy is here\n";


    SDL_Init(SDL_INIT_VIDEO); /**< SDL_INIT_VIDEO implies SDL_INIT_EVENTS , SDL_INIT_EVENTS: events subsystem*/
#ifdef IS_WEBGL2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES); //"es !!"
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);                        // desktop version automatically pick highest version, but in webversion we have to specify
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    // double buffer, bits for color, depth, stencil, multisampling

    gWindow = SDL_CreateWindow("CS200 Fun", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    gContext = SDL_GL_CreateContext(gWindow); // ask the grapics card to get an implementation of OpenGL, use glew library(GL, cross platform)

    SDL_GL_MakeCurrent(gWindow, gContext);

    glewInit();

    // Vsync
    constexpr int ADAPTIVE_SYNC = -1; // better sync
    constexpr int VSYNC         = 1;  // if adaptive sync is failed..
    if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_SYNC); result != 0)
    {
        SDL_GL_SetSwapInterval(VSYNC); // do classic vsync
    }
    setup();
#if !defined(__EMSCRIPTEN__)
    while (!gIsDone) // web - sometime it kills infinite loop so we have to use another func
    {
        main_loop();
    }
#else
    const bool simulate_infinte_loop   = true;
    const int  match_browser_framerate = -1; // match vsync rate
    emscripten_set_main_loop(main_loop, match_browser_framerate, simulate_infinte_loop);
#endif

    SDL_GL_DeleteContext(gContext);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) // loop for event
    {
        ImGuiHelper::FeedEvent(event);
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
                if (event.window.windowID == SDL_GetWindowID(gWindow)) // check if it is gwindow(not imgui window)
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        gIsDone = true;
                    }
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        gWidth  = event.window.data1;
                        gHeight = event.window.data2;
                    }
                }
                break;
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED: break;
            case SDL_QUIT: gIsDone = true; break;

            default: break;
        }
        //
    } // while loop for event



    // game logic
    glViewport(0, 0, gWidth, gHeight);
    demo_draw();


    [[maybe_unused]] const auto viewport = ImGuiHelper::Begin();
    demo_imgui();
    ImGuiHelper::End();

    // swap framebuffers
    SDL_GL_SwapWindow(gWindow);
}
