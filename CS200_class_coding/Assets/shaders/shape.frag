#version 300 es

precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

uniform sampler2D uTexture;

in vec2 vTexCoord;
in vec4 vTint;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 tex_color = texture(uTexture, vTexCoord);
    FragColor = tex_color * vTint;
}
