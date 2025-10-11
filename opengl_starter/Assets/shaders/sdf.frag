#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


in vec2 uTestPoint;

layout(location = 0) out vec4 FragColor;

uniform vec4 uFillColor;
uniform vec4 uLineColor;
uniform vec2 uWorldSize;
uniform float uLineWidth;
uniform int uShape;

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}

float sdRectangle( vec2 point, vec2 half_dim )
{
    vec2 d = abs(point)-half_dim;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

void main()
{

}
