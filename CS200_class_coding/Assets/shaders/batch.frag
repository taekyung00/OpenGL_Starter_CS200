#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

//every GPU have different amount limit of texture
//have to ask to OpenGL
uniform sampler2D uTexture;

in vec2 vTexCoord;
flat in vec4 vTint;

layout(location = 0) out vec4 FragColor;


void main()
{

    FragColor = texture(uTexture, vTexCoord) * vTint;

    if(FragColor.a == 0.0)
        discard;
}
