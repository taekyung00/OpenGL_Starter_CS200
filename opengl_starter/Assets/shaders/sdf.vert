#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aVertexPosition;

uniform mat3 uModel;
uniform mat3 uToNDC;//put uniform to use in c++ code !!
uniform vec2 uSDFScale;

out vec2 uTestPoint;
void main()
{
    vec3 ndc_point = uToNDC * uModel * vec3(aVertexPosition,1.0);
    gl_Position = vec4(ndc_point.xy, 0.0, 1.0);
    uTestPoint = aVertexPosition * uSDFScale; //scale
}
