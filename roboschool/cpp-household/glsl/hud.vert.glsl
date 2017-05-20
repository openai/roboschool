#version 330
//#line 3 "hud.vert.glsl"

uniform vec4 xywh;
uniform float zpos;

layout(location=0) in highp   vec4 input_vertex;

out vec2 texCoord;

void main()
{
    float x = xywh.x;
    float y = xywh.y;
    float w = xywh.z;
    float h = xywh.w;
    gl_Position = vec4(
        x + input_vertex.x*w,
        -y - input_vertex.y*h,
        0, 1.0);
    texCoord = vec2(
         (x + input_vertex.x*w) / 2 + 0.5,
         (y + input_vertex.y*h) / 2 + 0.5
        );
}
