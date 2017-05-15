//#line 2 "displaytex.frag.glsl"

uniform sampler2D inputTexture;

out vec4 out_Color;

in vec2 texCoord;

void main()
{
  out_Color = texture(inputTexture, texCoord);
  //out_Color.a = 0.5;
}
