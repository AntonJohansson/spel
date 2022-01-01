#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec2 pos;
layout(binding = 0) uniform uniform_buffer_object {
    vec2 pos;
    vec3 col;
} ubo;

layout(location = 0) out vec3 frag_col;

void main() {
    gl_Position = vec4(ubo.pos + pos, 0.0, 1.0);
    frag_col = ubo.col;
}
