#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 frag_offset;
layout(location = 1) in vec2 frag_size;
layout(location = 2) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    //vec2 v = frag_offset + frag_size * frag_tex_coord;
    //vec2 v = frag_offset + vec2(frag_size.x * (frag_tex_coord.x), frag_size.y*frag_tex_coord.y);
    vec2 v = frag_size*frag_tex_coord;
    //vec2 v = vec2(0.2, 0.2) + 0.1*frag_tex_coord;
    out_color = texture(tex_sampler, v);// vec4(frag_col, 1.0);
}
