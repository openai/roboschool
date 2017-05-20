#version 330
//#line 3 "simple_texturing.frag.glsl"




uniform bool enable_texture;
uniform sampler2D texture_id;

in Interpolants {
    //vec3 pos;
    vec3 normal;
    flat vec4 color;
    vec3 N;
    vec2 texcoord;
} IN;

layout(location=0,index=0) out vec4 out_Color;


void main()
{
    vec4 c = IN.color;
    //vec3 n1 = normalize(IN.normal);
    if (enable_texture) {
        c = texture(texture_id, IN.texcoord);
    }
    //out_Color   = (0.3 + 0.72*max(0.5, dot(IN.N, vec3(0,0,-1))) + 0.72*max(0.2, dot(vec3(n1), vec3(0,1,0))) ) * c;
    //out_Color   = (0.3 + 0.72*max(0.5, dot(IN.N, vec3(0,0,-1))) ) * c;
    //out_Color   = (0.3 + 0.72*max(0.2, dot(vec3(n1), vec3(0,1,0))) ) * c;
    if (false) {
        out_Color.r = IN.N[0];
        out_Color.g = IN.N[1];
        out_Color.b = IN.N[2];
        out_Color.a = 1;
    } else {
        float to_screen = max(0.0, dot(normalize(IN.N), vec3(0,0,-1)));
        out_Color = (0.9 + 0.1*pow(to_screen, 100)) * c;
        out_Color.a = c.a;
    }
}
