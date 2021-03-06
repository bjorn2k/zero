#version 330 core

uniform sampler2D modeltex;
in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_texcoord;
out vec3 outColor[2];

void main(void)
{
	outColor[0] = texture2D(modeltex, pass_texcoord).xyz;
	outColor[1] = vec3(texture2D(modeltex, pass_texcoord).xy, 1.0f);
}

