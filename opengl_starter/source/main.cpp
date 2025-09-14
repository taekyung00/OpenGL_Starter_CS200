/**
 * \file
 * \author Rudy Castan
 * \date 2024 Fall
 * \copyright DigiPen Institute of Technology
 */

#include <GL/glew.h>
#include <SDL.h>
#include <gsl/gsl> //owner template
#include <iostream>
#include "Handle.hpp"
#include "Shader.hpp"

gsl::owner<SDL_Window*>   gWindow  = nullptr; //mental reminder

#if defined(__EMSCRIPTEN__)
#include <emscripten.h> //for emscripten_set_main_loop

//./app_resources/web check html it assume we have setWindowSize
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(main_window)
{
    emscripten::function(
        "setWindowSize", emscripten::optional_override(
            [](int sizeX, int sizeY)
            {
                SDL_SetWindowSize(gWindow, sizeX, sizeY);
            }
        )
    );
}

#endif



gsl::owner<SDL_GLContext> gContext = nullptr; 
bool                      gIsDone  = false;
OpenGL::CompiledShader gShader;
OpenGL::Handle gVertexBuffer;
OpenGL::Handle gIndexBuffer;
OpenGL::Handle gVertexArrayObject; // handle to our model data
GLsizei                   gIndicesCount = 0;
int                       gWidth = 800;
int                       gHeight = 600;

void setup()
{
    const auto vertex_glsl = R"(#version 300 es
layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec3 aVertexColor;
out vec3 vColor;
void main()
{
    gl_Position = vec4(aVertexPosition, 0.0, 1.0);
    vColor = aVertexColor;
    
}
)";
    const auto fragment_glst = R"(#version 300 es
precision mediump float;

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";
    gShader                  = OpenGL::CreateShader(std::string_view{ vertex_glsl }, std::string_view{ fragment_glst });

    struct vertex
    {
        float x;
        float y;
        float r;
        float g;
        float b;
    };

    const vertex vertices[] = {
        // face verts
        {   -0.5,   -0.5, 1, 1, 0 },
        {   +0.5,   -0.5, 1, 1, 0 },
        {   +0.5,   +0.5, 1, 1, 0 },
        {   -0.5,   +0.5, 1, 1, 0 },
        // left eye
        { -0.375, +0.125, 0, 0, 0 },
        { -0.125, +0.125, 0, 0, 0 },
        { -0.125,  +0.25, 0, 0, 0 },
        { -0.375,  +0.25, 0, 0, 0 },
        // right eye
        { +0.125, +0.125, 0, 0, 0 },
        { +0.375, +0.125, 0, 0, 0 },
        { +0.375,  +0.25, 0, 0, 0 },
        { +0.125,  +0.25, 0, 0, 0 },
        // left mouth part
        { -0.375,  -0.25, 0, 0, 0 },
        {  -0.25,  -0.25, 0, 0, 0 },
        {  -0.25, -0.125, 0, 0, 0 },
        { -0.375, -0.125, 0, 0, 0 },
        // right mouth part
        {  +0.25,  -0.25, 0, 0, 0 },
        { +0.375,  -0.25, 0, 0, 0 },
        { +0.375, -0.125, 0, 0, 0 },
        {  +0.25, -0.125, 0, 0, 0 },
        // bottom middle mouth
        {  -0.25, -0.375, 0, 0, 0 },
        {  +0.25, -0.375, 0, 0, 0 },
    };

    // Triangle indices
    const unsigned short indices[] = { // face
                                       0, 1, 2, 0, 2, 3,
                                       // left eye
                                       4, 5, 6, 4, 6, 7,
                                       // right eye
                                       8, 9, 10, 8, 10, 11,
                                       // mouth
                                       // left mouth part
                                       12, 13, 14, 12, 14, 15, 12, 20, 13,
                                       // right mouth part
                                       16, 17, 18, 16, 18, 19, 16, 21, 17,
                                       // middle mouth part
                                       20, 21, 16, 20, 16, 13

    };
    gIndicesCount = static_cast<GLsizei>(std::ssize(indices));

    // buffer of vertex data
    glGenBuffers(1, &gVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // buffer of index data
    glGenBuffers(1, &gIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // vertex array object - model
    glGenVertexArrays(1, &gVertexArrayObject);
    glBindVertexArray(gVertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);


    // tell opengl how we org
    // describes our 2d position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),nullptr);
    glVertexAttribDivisor(0, 0);
    
    // describes our rgb color
    glEnableVertexAttribArray(1);
    ptrdiff_t offset = 2 * sizeof(float);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), reinterpret_cast<void*>(offset));
    glVertexAttribDivisor(1, 0);

    //un-select vao&buffers
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // associate our vertex&index buffers with this VAO
}
//w has to be 1, -1 to +1 with z
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); //desktop version automatically pick highest version, but in webversion we have to specify
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    // double buffer, bits for color, depth, stencil, multisampling

    gWindow = SDL_CreateWindow("CS200 Fun", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    gContext = SDL_GL_CreateContext(gWindow); //ask the grapics card to get an implementation of OpenGL, use glew library(GL, cross platform)

    SDL_GL_MakeCurrent(gWindow, gContext);

    glewInit();

    // Vsync
    constexpr int ADAPTIVE_SYNC = -1; //better sync
    constexpr int VSYNC         = 1; //if adaptive sync is failed..
    if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_SYNC); result != 0)
    {
        SDL_GL_SetSwapInterval(VSYNC); //do classic vsync
    }
    setup();
#if !defined(__EMSCRIPTEN__)
    while (!gIsDone) //web - sometime it kills infinite loop so we have to use another func
    {
       main_loop();
    }
#else
    const bool simulate_infinte_loop = true;
    const int match_browser_framerate = -1; //match vsync rate
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
    while (SDL_PollEvent(&event) != 0)  //loop for event
    {
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                {
                    gIsDone = true;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    gWidth  = event.window.data1;
                    gHeight = event.window.data2;
                }
                break;
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED: 
                
                break;
            case SDL_QUIT: gIsDone = true; break;

            default: break;
        }
        //
    }//while loop for event

    // game logic
    // drawing with opengl
    glViewport(0, 0, gWidth, gHeight);
    glClearColor(0.34f, 0.56f, 0.9f, 1.0f); // just 'set' window color
    glClear(GL_COLOR_BUFFER_BIT); //actually clear

    // drawing
    glUseProgram(gShader.Shader);
    glBindVertexArray(gVertexArrayObject);

    glDrawElements(GL_TRIANGLES, gIndicesCount, GL_UNSIGNED_SHORT, nullptr);

    glUseProgram(0);
    glBindVertexArray(0);

    // swap framebuffers - double buffer, so when drawing is done, now it's time to show on screen, painter metaphor..
    SDL_GL_SwapWindow(gWindow);
}
