#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec3 frag_col;
layout(location = 1) in vec2 frag_tex_coord;

layout(binding = 0) uniform sampler2D tex_sampler;

void main() {
    out_color = texture(tex_sampler, frag_tex_coord);// vec4(frag_col, 1.0);
}
