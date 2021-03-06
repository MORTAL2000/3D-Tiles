#version 440 core

//A quick test shader for glMultiDrawElementsIndirect

layout(std140) uniform Viewing {
	mat4 view, proj;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in mat4 model;

out vec3 fcolor;

void main(void){
	fcolor = color + 0.5 * normal;
	gl_Position = proj * view * model * vec4(pos, 1);
}

