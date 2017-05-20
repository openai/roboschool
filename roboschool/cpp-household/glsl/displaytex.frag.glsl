#version 330
//#line 3 "displaytex.frag.glsl"    // mac doesn't like #line instruction

uniform sampler2D inputTexture;

out vec4 out_Color;

in vec2 texCoord;

void main()
{
  out_Color = texture(inputTexture, texCoord);
  //out_Color.a = 0.5;
}
