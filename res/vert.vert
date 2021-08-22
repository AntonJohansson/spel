#version 450
#extension GL_ARB_separate_shader_objects: enable

vec2 pos[3] = {
	vec2(0.0, -0.5),
	vec2(0.5,  0.5),
	vec2(-0.5, 0.5)
};

vec3 col[3] = {
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
};

layout(location = 0) out vec3 frag_col;

void main() {
	gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
	frag_col = col[gl_VertexIndex];
}
