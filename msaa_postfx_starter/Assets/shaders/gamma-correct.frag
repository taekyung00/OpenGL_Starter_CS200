#version 300 es
precision mediump float;
/**
 * \file
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
uniform sampler2D uColorTexture;
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

uniform float uGamma;

void main()
{
    FragColor = texture(uColorTexture, vTexCoord);
    FragColor.rgb = pow(FragColor.rgb, vec3(uGamma));
}
