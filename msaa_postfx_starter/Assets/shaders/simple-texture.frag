#version 300 es

precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

uniform sampler2D uColorTexture;

in vec2 vTexCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(uColorTexture, vTexCoord);
}
