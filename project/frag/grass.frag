
#version 330 core  

out vec4 FragColor;  
precision mediump float;   
uniform sampler2D grassSampler;   
in vec2 texCoords;   

in vec3 fragPos;
in vec3 fragNormal;
#define lamp_count 10 

uniform vec3 u_view_pos;
uniform vec3 u_light_pos[lamp_count];
uniform vec3 u_light_colour[lamp_count];
uniform float ambient_strength;
uniform float diffuse_strength;
uniform float specular_strength;
uniform float shininess;
uniform float constant;
uniform float linear;
uniform float quadratic;


 void main() {   

	vec3 basecolor = texture(grassSampler, texCoords).rgb;
	
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(u_view_pos - fragPos);
	
	
	vec3 ambient = ambient_strength * basecolor;
	vec3 phong = vec3(0.0f);

	for (int i=0; i<lamp_count; i++){
		vec3 L = normalize(u_light_pos[i] - fragPos);
		vec3 R = reflect(-L, N);

		float distance = length(u_light_pos[i] - fragPos);
		float attenuation = 1.0/(constant +linear*distance +quadratic*distance*distance );

		vec3 diffuse = diffuse_strength * max(dot(N,L), 0.0) * u_light_colour[i] *basecolor;

		vec3 specular  = specular_strength * pow(max(dot(V,R),0.0), shininess)*u_light_colour[i];
		
		phong +=  diffuse *attenuation + specular*attenuation;
	}


	FragColor =  vec4(ambient + phong, 1.0 );
 }   
