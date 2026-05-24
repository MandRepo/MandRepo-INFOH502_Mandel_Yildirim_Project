#version 330 core


in vec3 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 M;

void main(){
	gl_Position = lightSpaceMatrix * M * vec4(position , 1.0);
}

