#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aWorldPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aTint;
layout(location = 3) in int aTextureIndex;
layout(location = 4) in float aDepth;

out vec2 vTexCoord;
//by default, any output variable interpolated
//but vTint has to be same across the triangle(for each pixels)
//so put flat
flat out vec4 vTint;
flat out int vTextureIndex;

// uniform mat3 uModel; //get rid of it so that cpu do this 
uniform mat3 uViewNDC;
// uniform mat3 uTexCoordTransform; //get rid of it so that cpu do this 

/**
why give model xform task to CPU?
we have lots of vertex(duplicated)
it is more 
*/

void main()
{
    vec3 ndc_point = uViewNDC * /* uModel * */ vec3(aWorldPosition, 1.0); //we assume that vertex position is already in world space(we dont need model xformation)
    gl_Position = vec4(ndc_point.xy, aDepth, 1.0);
    vTexCoord = aTexCoord.st;//don't need texcoord xform anymore, just copy
    vTint = aTint;
    vTextureIndex = aTextureIndex;
}
