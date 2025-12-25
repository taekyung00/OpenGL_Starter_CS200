#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aVertexPosition;
//if doing 3D, depth is provided as model coordinate so we have to use vec3 aVertexPosition; 
// but we are doing 2D here so just keep it vec2, and use uniform value of depth
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat3 uModel;
uniform mat3 uViewNDC;
//uniform mat3 uNDC;
//uniform mat3 uView;
uniform mat3 uTexCoordTransform;
uniform float uDepth; // only needed for 2D rendering


void main()
{
    vec3 ndc_point = uViewNDC * uModel * vec3(aVertexPosition, 1.0);
    // vec3 ndc_point = uNDC * uView* uModel * vec3(aVertexPosition, 1.0);
    gl_Position = vec4(ndc_point.xy, uDepth, 1.0);
    // gl_Position expects its value in NDC space, so uDepth has to be in [-1, 1] range
    // if out of this range, clipping will occur and possibly nothing will be rendered
    vec3 tex_coords = uTexCoordTransform * vec3(aTexCoord, 1.0);
    vTexCoord = tex_coords.st;
}
