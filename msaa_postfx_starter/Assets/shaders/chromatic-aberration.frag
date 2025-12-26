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

uniform vec2 uMouseFocusPoint; // measure in texture coords 0-1

void main()
{
    float red_offset = 0.009;
    float green_offset = 0.006;
    float blue_offset = -0.006;
    vec2 direction = vTexCoord - uMouseFocusPoint;
    FragColor.r = texture(uColorTexture, vTexCoord + (direction*vec2(red_offset))).r;
    FragColor.g = texture(uColorTexture, vTexCoord + (direction*vec2(green_offset))).g;
    FragColor.b = texture(uColorTexture, vTexCoord + (direction*vec2(blue_offset))).b;
    FragColor.a = texture(uColorTexture, vTexCoord).a;

}