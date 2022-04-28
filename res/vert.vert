#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coord;

layout(push_constant) uniform Constants {
    vec2 pos;
    vec3 col;
} push;

layout(location = 0) out vec3 frag_col;
layout(location = 1) out vec2 frag_tex_coord;

void main() {
    gl_Position = vec4(push.pos + pos, 0.0, 1.0);
    frag_col = push.col;
    frag_tex_coord = tex_coord;
}
