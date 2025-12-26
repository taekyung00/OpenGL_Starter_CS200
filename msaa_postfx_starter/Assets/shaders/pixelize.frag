#version 300 es
precision mediump float;
/**
 * \file
 * \author Taekyung HoeWhore
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
uniform sampler2D uColorTexture;
in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

uniform int pixelSize;

void main() {
    FragColor = texture(uColorTexture, vTexCoord);

  // Must be odd.
  if(pixelSize % 2 == 0 || pixelSize <= 1) 
    return;

  vec2 tex_size = vec2(textureSize(uColorTexture, 0));
  vec2 texCoord = gl_FragCoord.xy / tex_size;


  float x = float(int(gl_FragCoord.x) % pixelSize);
  float y = float(int(gl_FragCoord.y) % pixelSize);

        x = floor(float(pixelSize) / 2.0) - x;
        y = floor(float(pixelSize) / 2.0) - y;

        x = gl_FragCoord.x + x;
        y = gl_FragCoord.y + y;

  vec2 uv = vec2(x, y) / tex_size;

  FragColor = texture(uColorTexture, uv);
}