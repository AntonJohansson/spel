#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 frag_offset;
layout(location = 1) in vec2 frag_size;
layout(location = 2) in vec2 frag_tex_coord;
layout(location = 3) in vec3 frag_color;

layout(location = 0) out vec4 out_color;

void main() {
    vec2 v = frag_offset + frag_size*frag_tex_coord;
    float s = texture(tex_sampler, v).r;
    out_color = vec4(s*frag_color, s);
}
