#version 330
//#line 3 "simple_texturing.vert.glsl"

uniform highp mat4 input_matrix_modelview;
uniform highp mat4 input_matrix_modelview_inverse_transpose;
uniform highp vec4 uni_color;
uniform bool enable_texture;

layout(location=0) in highp   vec4 input_vertex;
layout(location=1) in mediump vec4 input_normal;
layout(location=2) in mediump vec2 input_texcoord;

out Interpolants {
    //vec3 pos;
    vec3 normal;
    flat vec4 color;
    vec3 N;
    vec2 texcoord;
} OUT;

void main(void)
{
    //mat4 t = input_matrix_modelview_inverse_transpose;
    OUT.N = vec3( normalize(mat3(input_matrix_modelview_inverse_transpose) * vec3(input_normal)) );
    gl_Position = input_matrix_modelview * input_vertex;
    //OUT.pos = vec3(input_vertex);
    //vec3(input_vertex.x, input_vertex.y, input_vertex.z);
    OUT.normal = vec3(input_normal);
    OUT.color = uni_color;
    if (enable_texture) {
        OUT.texcoord = vec2(input_texcoord);
    }
}
