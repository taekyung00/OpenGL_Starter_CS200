#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

uniform sampler2D uTexuture;

in vec2 vTexCoord;

layout(location = 0) out vec4 FragColor;

uniform vec4 uTint; //  cover with new color

void main()
{
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    FragColor = texture(uTexuture, vTexCoord) * uTint; //it will return vec of rgba
    if(FragColor.a == 0.0) 
    discard;//discard background
}
