#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */


in vec2 vTestPoint;

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

//evalute the color based off sdf
vec4 evalute_color(float sdf)
{
    float fill_alpha = (sdf < 0.0) ? 1.0 : 0.0;
    float outline_alpha = (abs(sdf) < uLineWidth * 0.5) ? 1.0 : 0.0;

    vec4 fill_color = vec4(uFillColor.rgb, fill_alpha * uFillColor.a);//modulate,mix
    vec4 line_color = vec4(uLineColor.rgb, outline_alpha * uLineColor.a);//modulate,mix

    if(line_color.a > 0.0)
        return line_color;
    return fill_color;
}

void main()
{
    //based off shape evaluate the sdf
    float sdf = 0.0;
    if(uShape == 0){
        float radius = min(uWorldSize.x ,uWorldSize.y) * 0.5;
        sdf = sdCircle(vTestPoint, radius);
    }
    else if(uShape == 1){
        sdf = sdRectangle(vTestPoint, 0.5 * uWorldSize);
    }
    
    //get the color
    vec4 color = evalute_color(sdf);
    if(color.a <= 0.0 )
        discard;
    //set color, discard empty space
    FragColor = color;
}
