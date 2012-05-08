#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;
layout (location = 4) uniform mat4 view;
out vec3 pass_position;
out vec3 pass_normal;
out vec2 pass_texcoord;

void main(void)
{
	gl_Position = view * vec4(in_position, 1.0);
	pass_position = in_position;
	pass_normal = vec4(in_normal, 0.0f).xyz;
	pass_texcoord = in_texcoord;
}

