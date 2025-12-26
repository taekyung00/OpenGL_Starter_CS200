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

uniform int uBlurSize;
uniform float uSeparation;

void main()
{
    FragColor = texture(uColorTexture, vTexCoord);
    if(uBlurSize <= 0){
        return;
    }
    vec2 tex_size = vec2(textureSize(uColorTexture, 0));
    FragColor = vec4(vec3(0.0), FragColor.a);
    // -size to +size, (2*size+1) * (2*size+1)
    for(int i = -uBlurSize; i <= uBlurSize; ++i)
    {
        for(int j = -uBlurSize; j < uBlurSize; ++j)
        {
            vec2 offset = vec2(i,j) * uSeparation / tex_size;
            FragColor.rgb += texture(uColorTexture, vTexCoord+offset).rgb;
        }
    }
    float count = (2.0*float(uBlurSize)+1.0);
    count *= count;
    FragColor.rgb /= count;
}