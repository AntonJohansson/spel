#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

// CONTHERE: Maybe something with padding?
layout(push_constant) uniform Constants {
    vec2 pos;
    vec2 scale;
    vec3 col;
    vec2 offset;
    vec2 size;
} push;

layout(location = 0) out vec2 frag_offset;
layout(location = 1) out vec2 frag_size;
layout(location = 2) out vec2 frag_tex_coord;

void main() {
    gl_Position = vec4(push.pos + push.scale * pos, 0.0, 1.0);
    frag_offset = push.offset;
    frag_size = push.size;
    frag_tex_coord = tex_coord;
}
