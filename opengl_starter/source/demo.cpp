#include "Path.hpp"
#include "Random.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array>   //feed array to vertex shader, and vertex shader do NDC
#include <imgui.h>
#include <vector>

extern int gWidth;
extern int gHeight;

void demo_setup()
{
}

void demo_draw()
{
}

void demo_imgui()
{
    ImGui::Begin("Demo Settings");
    ImGui::End();
}
