#version 330 core
out vec4 FragColor;
precision mediump float; 
in vec3 v_frag_coord; 
in vec3 v_normal; 

#define lamp_count 10 

uniform vec3 u_view_pos; 
uniform vec3 u_light_pos[lamp_count];
uniform vec3 u_light_colour[lamp_count];
uniform samplerCube cubemapSampler; 
uniform float ambient_strength;
uniform float diffuse_strength;
uniform float specular_strength;
uniform float shininess;
uniform float constant;
uniform float linear;
uniform float quadratic;
uniform int useRefraction;
float bias = 0.0001;

uniform sampler2D shadowMap;
in vec4 v_fragPosLightSpace;

void main() { 

	vec3 N = normalize(v_normal); 
	vec3 V = normalize(u_view_pos - v_frag_coord);
	vec3 baseColor =  vec3(1.0, 0.0, 0.0) ;
	vec3 R ;
	//reflect cubemap on the car
	if (useRefraction == 1) {
		R = reflect(-V, N);
	}
	else{
		R = refract(-V, N, 1.0f/1.52f);
	}
	vec3 Reflec = texture(cubemapSampler, R).rgb; 
	vec3 ambient = ambient_strength * baseColor ;

	vec3 totalLight = vec3(0.0f);

	for (int i=0; i<lamp_count; i++){
		
		vec3 L = normalize(u_light_pos[i] - v_frag_coord);
		//Light of phong
		vec3 Rspec = reflect(-L, N);
		float distance = length(u_light_pos[i] - v_frag_coord);
		float attenuation = 1.0/(constant + linear*distance + quadratic*distance*distance );

		vec3 diffuse = diffuse_strength * max(dot(N,L), 0.0) * u_light_colour[i] * baseColor;
		vec3 specular  = specular_strength * pow(max(dot(V,Rspec),0.0), shininess)*u_light_colour[i];

		totalLight +=   diffuse *attenuation + specular*attenuation;
		}

	//Compute the shadow from the car 
	vec3 projCoords = v_fragPosLightSpace.xyz / v_fragPosLightSpace.w ;
	projCoords = projCoords * 0.5 + 0.5;
	float closeDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	
	float shadow;
	if (currentDepth - bias > closeDepth && projCoords.z <= 1.0f){
		shadow = 1.0f;

	}else{
		shadow = 0.0f;

	}
	
	vec3 colorFin =  totalLight + 0.3f * Reflec ;
	FragColor =  vec4(ambient *(1.0- shadow*0.2) + colorFin * (1.0- shadow*0.7), 1.0 );

	
		

}

