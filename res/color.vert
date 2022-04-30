#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec2 pos;

layout(push_constant) uniform Constants {
    vec2 pos;
    vec2 scale;
    vec3 col;
    vec2 offset;
    vec2 size;
} push;

layout(location = 0) out vec3 frag_col;

void main() {
    gl_Position = vec4(push.pos + push.scale * pos, 0.0, 1.0);
    frag_col = push.col;
}
