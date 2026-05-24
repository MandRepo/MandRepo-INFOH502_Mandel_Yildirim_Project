#version 330 core
in vec3 position; 
in vec2 tex_coords; 
in vec3 normal; 

out vec2 texCoords; 
out vec3 fragPos; 
out vec3 fragNormal; 

uniform mat4 M;
uniform mat4 itM; 
uniform mat4 V; 
uniform mat4 P; 


void main(){ 
	gl_Position = P*V*M* vec4(position, 1.0); 
	texCoords = tex_coords;
	fragPos = vec3(M * vec4(position, 1.0));
	fragNormal =  normalize(mat3(itM)*normal);

}