#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec2 aTexCoord; //and put this to fragment shader

out vec2 vTexCoord;

uniform mat3 uModel;
uniform mat3 uToNDC;//put uniform to use in c++ code !!
uniform mat3 uTexCoordTransform; //for drawing just part of image

void main()
{
    vec3 ndc_point = uToNDC * uModel * vec3(aVertexPosition,1.0);
    gl_Position = vec4(ndc_point.xy, 0.0, 1.0);
    vec3 tex_coords = uTexCoordTransform * vec3(aTexCoord,1.0);
    vTexCoord = tex_coords.st; //same as xy
}
