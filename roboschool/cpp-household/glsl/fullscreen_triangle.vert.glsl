#version 330
//#line 3 "fullscreen_triangle.vert.glsl"

out vec2 texCoord;

void main()
{
  uint idx = uint(gl_VertexID % 3);
  vec4 pos =  vec4(
      (float( idx     &1U)) * 4.0 - 1.0, // one big triangle stratches further than -1 (not to use quad)
      (float((idx>>1U)&1U)) * 4.0 - 1.0,
      0, 1.0);
  gl_Position = pos;
  texCoord = pos.xy * 0.5 + 0.5;
}
