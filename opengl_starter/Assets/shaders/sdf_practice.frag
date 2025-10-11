#version 300 es
precision mediump float;

uniform float u_time;
uniform vec2 u_resolution;

out vec4 FragColor;

// note: set up basic colors
  vec3 black = vec3(0.0);
  vec3 white = vec3(1.0);
  vec3 red = vec3(1.0, 0.0, 0.0);
  vec3 blue = vec3(0.65, 0.85, 1.0);
  vec3 orange = vec3(0.9, 0.6, 0.3);


//circle

// https://iquilezles.org/articles/distfunctions2d/
float sdfCircle(vec2 p, float r) {
  // note: sqrt(pow(p.x, 2.0) + pow(p.y, 2.0)) - r;
  return length(p) - r;
}

void main() {
  // note: set up uv coordinates - it differ for each pixel
//   vec2 uv = gl_FragCoord.xy / u_resolution;//normalization [0.0,1.0]
  //uv = uv - 0.5; //make center as 0,0, uv is now -0.5 to 0.5
  //uv *= u_resolution ; //scale to screen with resolution, and divide with 100 so we can see clearly

    vec2 uv = gl_FragCoord.xy - 0.5*u_resolution; //same as above
  
  vec3 color = black;
  color = vec3(uv.x, uv.y, 0.0);

//   // note: draw circle sdf
  float radius = 128.0;
  vec2 center = vec2(0.0, 0.0);
//   center = vec2(sin(2.0 * u_time), 0.0);
  float distanceToCircle = sdfCircle(uv - center, radius);
  color = distanceToCircle > 0.0 ? orange : blue;

  if(color.a < 0.0)
    discard;


  FragColor = vec4(color, 1.0);
}




//rectangle
// void main()
// {
//     vec2 uv = gl_FragCoord.xy / u_resolution;//normalization [0.0,1.0]
//     uv = uv - 0.5; //make center as 0,0
//     uv *= u_resolution; //scale to screen with resolution, and divide with 100 so we can see clearly
//     vec2 half_dim = vec2(100.0, 100.0);
//     vec2 distance_to_edges = abs(uv) - half_dim; //use abs so we can focus 1st quad
//     float outside_distance = length(max(distance_to_edges, 0.0)); //scalar 0.0 is extended to vec2, 0.0,0.0//x > 0 or y > 0 -> outside!
//     //max(vec2 v1, vec2 v2) -> return vec2(max(v1.x, v2.x), max(v1.y, v2.y)) 
//     float inside_distance = min(max(distance_to_edges.x, distance_to_edges.y)/*shorter distance*/, 0.0/*works only negative*/);
//     float sdf = outside_distance + inside_distance;
//     vec4 fill_color  = sdf > 0.f ?  vec4(0.5333, 0.8549, 0.5333, 1.0) : vec4(0.7451, 0.1098, 0.7804, 1.0);
//     float line_thickness = 20.0;
//     vec4 line_color = (abs(sdf) < line_thickness*0.5) ? vec4(0.8039, 1.0, 0.0196, 1.0) : vec4(0.0);

//     if(line_color.a > 0.0)
//         FragColor = line_color;
//     else
//         FragColor = fill_color;
//     // FragColor = vec4(vec3(sdf/(0.5*min(u_resolution.x, u_resolution.y))), 1.0);
// }

//segment (line)

// void main()
// {
//     vec2 uv = gl_FragCoord.xy / u_resolution;//normalization [0.0,1.0]
//     uv = uv - 0.5; //make center as 0,0
//     uv *= u_resolution; //scale to screen with resolution, and divide with 100 so we can see clearly

//     vec2 a = vec2(10.0, 10.0);
//     vec2 b = vec2(100.0, 100.0);

//     vec2 pa = uv - a, ba = b-a; //pa is vec of uv-a
//     float h = clamp(dot(pa,ba) / dot(ba, ba) , 0.0, 1.0); //projection pa to ba and clamp between 0.0 - 1.0
//     float line_thickness = 1.f;
//     float line_sdf = length(uv-(a + ba * h)); //a - ba * h is Q -> dot between A and B
//     //pa - ba * h == p - (a + h * ba)
//     vec4 line_color = line_sdf > line_thickness? vec4(0.5333, 0.8549, 0.5333, 1.0) : vec4(0.0431, 0.0392, 0.0431, 1.0);
//     FragColor = line_color;
    
// }