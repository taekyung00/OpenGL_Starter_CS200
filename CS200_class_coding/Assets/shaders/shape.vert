#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aTint;

out vec2 vTexCoord;
out vec4 vTint;

uniform mat3 uToNDC;
uniform mat3 uTransform;

void main()
{
    // Apply transform matrix to position
    vec3 transformed_pos = uTransform * vec3(aPosition, 1.0);
    
    // Apply NDC matrix
    vec3 ndc_point = uToNDC * transformed_pos;
    
    gl_Position = vec4(ndc_point.xy, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vTint = aTint;
}
