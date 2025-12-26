#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    // Vertices are already in NDC space, just pass through
    gl_Position = vec4(aVertexPosition, 0.0, 1.0);
    vTexCoord   = aTexCoord;
}
