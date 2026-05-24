#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_inverse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <map>
#include <vector>
#include "camera.h"
#include "shader.h"
#include "object.h"
#include "shadow.h"

#include <fstream>
#include <sstream>

#include <algorithm>
#include <chrono>
#include <thread>
#include <glm/gtx/color_space.hpp>

const int width = 1920;
const int height = 1080;
float speed = 0.0f;
float acc_actu = 0.00f;

float rotspeed = 0.05f;
float acc_std = 0.01f;
float driftDirection = 0.0f;
bool isDrift = false;
float rot_direction = 1.0f;
bool refract_actif = false;
bool R_press = false;

bool isPaused = false;
bool p_press = false;

glm::vec3 car_coord_prev = glm::vec3(0.0f);

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window, glm::vec3& car_coord, float& carYaw);

void loadCubemapFace(const char* file, const GLenum& targetCube);

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif

//Define the camera
Camera camera(glm::vec3(0.0, 0.0, 0.1));

std::string readShaderFile(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Error : file doesn\'t open" << path<<std::endl;
		return "";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

struct Lampad {
	glm::vec3 position;
	glm::vec3 light_pos;
	glm::vec3 colour;
	glm::mat4 model;
	glm::mat4 invModel;

};



//déclaration of the particles
struct Particle {
	glm::vec3 pos, speed;
	glm::vec4 color;
	float life;
	float size;
	float cameraDist; //Squared distance to the camera position

	Particle() : pos(0.0f), speed(0.0f), color(1.0f), life(0.0f), size(0.0f), cameraDist(0.0f) {}

	bool operator<(const Particle& otherP) const {
		//sort in reverse order, first particles that are further away
		return this->cameraDist > otherP.cameraDist;
	}
};



const int MaxParticles = 1000000;
Particle particlesContainer[MaxParticles];

int lastUsedParticle = 0;

int findUnusedParticle() {
	for (int i = lastUsedParticle; i < MaxParticles; i++) {
		if (particlesContainer[i].life < 0) {
			lastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < lastUsedParticle; i++) {
		if (particlesContainer[i].life < 0) {
			lastUsedParticle = i;
			return i;
		}
	}

	return 0;
}

void sortParticles() {
	std::sort(&particlesContainer[0], &particlesContainer[MaxParticles]);
}


int main(int argc, char* argv[])
{
	std::cout << "Welcome to Project by Mandel ELiot and Yildirim Emirhan: " << std::endl;
	std::cout << "Implement relection on an object\n"
		"\n";


	//Boilerplate
	//Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	//create a debug context to help with Debugging
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	//Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Project H-502", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);

	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	// add the wall
	const std::string sourceVwall = readShaderFile(PATH_TO_VERT "/wall.vert");
	const std::string sourceFwall = readShaderFile(PATH_TO_FRAG "/wall.frag");
	Shader wallShader = Shader(sourceVwall, sourceFwall);
	char pathWall[] = PATH_TO_OBJECTS "/wall.obj";
	Object wall(pathWall);
	wall.makeObject(wallShader);
	glm::mat4 wallMod = glm::mat4(1.0f);
	wallMod = glm::translate(wallMod, glm::vec3(19.5f, 0.3f, -15.0f));
	wallMod = glm::rotate(wallMod, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//wallMod = glm::rotate(wallMod, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	wallMod = glm::scale(wallMod, glm::vec3(0.5f, 1.0f, 1.0f));
	glm::mat4 wallInv = glm::transpose(glm::inverse(wallMod));
	

	// add the car
	const std::string sourceV = readShaderFile(PATH_TO_VERT "/car.vert");
	const std::string sourceF = readShaderFile(PATH_TO_FRAG "/car.frag");
	Shader shader(sourceV, sourceF);
	char path[] = PATH_TO_OBJECTS "/little_car.obj";
	Object car(path);
	car.makeObject(shader);
	
	//add the bunny linked with the shader from the car
	
	char pathBunny[] = PATH_TO_OBJECTS "/bunny_small.obj";
	Object bunny(pathBunny);
	bunny.makeObject(shader);

	glm::mat4 bunnyMod = glm::mat4(1.0f);
	bunnyMod = glm::translate(bunnyMod, glm::vec3(-10.0f, 1.0f, -10.0f));
	bunnyMod = glm::scale(bunnyMod, glm::vec3(1.0f,1.0f, 1.0f));
	glm::mat4 bunnyInv = glm::transpose(glm::inverse(bunnyMod));


	// add the cubemap
	const std::string sourceVCubeMap = readShaderFile(PATH_TO_VERT "/cubemap.vert");
	const std::string sourceFCubeMap = readShaderFile(PATH_TO_FRAG "/cubemap.frag");
	Shader cubeMapShader = Shader(sourceVCubeMap, sourceFCubeMap);
	char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
	Object cubeMap(pathCube);
	cubeMap.makeObject(cubeMapShader);

	// Load the .obj of the ground
	const std::string sourceVGround = readShaderFile(PATH_TO_VERT "/ground.vert");
	const std::string sourceFGround = readShaderFile(PATH_TO_FRAG "/ground.frag");
	Shader groundShader = Shader(sourceVGround, sourceFGround);
	char pathGround[] = PATH_TO_OBJECTS "/race_road.obj";
	Object ground(pathGround);
	ground.makeObject(groundShader);

	glm::mat4 groundMod = glm::mat4(1.0f);
	groundMod = glm::translate(groundMod, glm::vec3(0.0f, -0.2f, 0.0f));
	groundMod = glm::scale(groundMod, glm::vec3(2.0f, 1.0f, 2.0f));
	glm::mat4 groundInv = glm::transpose(glm::inverse(groundMod));

	// add the light on the lamp
	const std::string sourceVpole = readShaderFile(PATH_TO_VERT "/lamp.vert");
	const std::string sourceFpole = readShaderFile(PATH_TO_FRAG "/lamp.frag");
	Shader lampShader = Shader(sourceVpole, sourceFpole);
	char pathLamp[] = PATH_TO_OBJECTS "/lamp.obj";
	Object lamp(pathLamp);
	lamp.makeObject(lampShader);

	auto computeLamp = [&](glm::vec3 pos,float rotation, glm::vec3 pos_light, glm::vec3 light_color) {
		Lampad l;
		l.position = pos;
		l.light_pos = pos + pos_light;
		l.model = glm::translate(glm::mat4(1.0f), pos);
		l.model = glm::rotate(l.model, glm::radians(rotation), glm::vec3(0.00f, 1.0f, 0.00f));
		l.model = glm::scale(l.model, glm::vec3(0.05f, 0.08f, 0.05f));
		l.invModel = glm::transpose(glm::inverse(l.model));
		l.colour = light_color;
		return l;
	};

	std::vector<Lampad> lampadaires;
	lampadaires.push_back(computeLamp(glm::vec3(-7.5f, 0.4f, 2.3f), 0.0f, glm::vec3(0.0f, 0.95f, -0.3f), glm::vec3(1.0f, 0.0f, 0.0f)));       // Red
	lampadaires.push_back(computeLamp(glm::vec3(-32.0f, 0.4f, -32.0f), 50.0f, glm::vec3(-0.2f, 0.95f, -0.2f), glm::vec3(1.0f, 1.0f, 0.0f)));  //Yellow
	lampadaires.push_back(computeLamp(glm::vec3(-32.0f, 0.4f, -9.0f), -50.0f, glm::vec3(0.2f, 0.95f, -0.2f), glm::vec3(0.0f, 1.0f, 0.0f)));   //green
	lampadaires.push_back(computeLamp(glm::vec3(-0.5f, 0.4f, -36.5f), 105.0f, glm::vec3(-0.3f, 0.95f, 0.1f), glm::vec3(0.53f, 0.8f, 1.58f))); //blue
	lampadaires.push_back(computeLamp(glm::vec3(-5.5f, 0.4f, -46.5f), -45.0f, glm::vec3(0.2f, 0.95f, -0.25f), glm::vec3(1.0f, 0.52f, 1.0f))); //pink
	lampadaires.push_back(computeLamp(glm::vec3(14.0f, 0.4f, -15.0f), -90.0f, glm::vec3(0.25f, 0.95f, -0.0f), glm::vec3(0.87f, 0.0f, 0.4f))); //pink

	
	

	// add the sphereto lamp --> To delete
	const std::string sourceVsphere = readShaderFile(PATH_TO_VERT "/light.vert");
	const std::string sourceFsphere = readShaderFile(PATH_TO_FRAG "/light.frag");
	Shader lightShader = Shader(sourceVsphere, sourceFsphere);
	char pathLight[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
	Object lightSphere(pathLight);
	lightSphere.makeObject(lightShader);

	//add the sphere of the moon
	Shader moonShader = Shader(sourceVsphere, sourceFsphere);
	Object moonSphere(pathLight);
	moonSphere.makeObject(moonShader);



	// add the ground and grass
	const std::string sourceVgrass = readShaderFile(PATH_TO_VERT "/grass.vert");
	const std::string sourceFgrass = readShaderFile(PATH_TO_FRAG "/grass.frag");
	Shader grassShader = Shader(sourceVgrass, sourceFgrass);
	char pathGrass[] = PATH_TO_OBJECTS "/plane.obj";
	Object grass(pathGrass);
	grass.makeObject(grassShader);
	glm::mat4 grassMod = glm::mat4(1.0f);
	grassMod = glm::translate(grassMod, glm::vec3(0.0f, -0.25f, 0.0f));
	grassMod = glm::scale(grassMod, glm::vec3(65.0f, 1.0f, 65.0f));
	glm::mat4 grassInv = glm::transpose(glm::inverse(grassMod));


	//add the shader for the particles

	const std::string sourceVpart = readShaderFile(PATH_TO_VERT "/part.vert");
	const std::string sourceFpart = readShaderFile(PATH_TO_FRAG "/part.frag");
	Shader particleShader = Shader(sourceVpart, sourceFpart);

	
	//Compute the FPS on live
	double prev = 0;
	int deltaFrame = 0;
	//fps function
	auto fps = [&](double now) {
		double deltaTime = now - prev;
		deltaFrame++;
		if (deltaTime > 0.5) {
			prev = now;
			const double fpsCount = (double)deltaFrame / deltaTime;
			deltaFrame = 0;
			std::cout << "\r FPS: " << fpsCount;
			std::cout.flush();
		}
	};

	//Set the camera
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();
	glm::vec3 car_coord = glm::vec3(0.0f, 0.0f, 1.0f);
	float carYaw = 0.0f;

	//First compute of the car
	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0, 0.0, -2.0));
	model = glm::scale(model, glm::vec3(0.2, 0.1, 0.2));
	glm::mat4 inverseModel = glm::transpose(glm::inverse(model));

	// Position of the light


	float ambient = 0.22;
	float diffuse = 0.5;
	float specular = 0.8;

	glm::vec3 materialColour = glm::vec3(0.5f, 0.6, 0.8);

	//Rendering
	auto setupLightUnifroms = [&](Shader& sh) {
		sh.use();
		sh.setFloat("shininess", 32.0f);
		sh.setVector3f("materialColour", materialColour);
		sh.setFloat("ambient_strength", ambient);
		sh.setFloat("diffuse_strength", diffuse);
		sh.setFloat("specular_strength", specular);
		sh.setFloat("constant", 1.0);
		sh.setFloat("linear", 0.14);
		sh.setFloat("quadratic", 0.07);
		for (int i = 0; i<(int)lampadaires.size(); i++) {
			sh.setVector3f(("u_light_pos[" + std::to_string(i) + "]").c_str(), lampadaires[i].light_pos);
			sh.setVector3f(("u_light_colour["+std::to_string(i)+"]").c_str(), lampadaires[i].colour);

		}
	};

	setupLightUnifroms(shader);
	setupLightUnifroms(groundShader);
	setupLightUnifroms(wallShader);
	setupLightUnifroms(lampShader);
	setupLightUnifroms(grassShader);



	//Apply the light on the concerns objects
	groundShader.use();
	groundShader.setFloat("specular_strength", 0.0f);
	shader.use();
	shader.setFloat("specular_strength", 0.0f);
	grassShader.use();
	grassShader.setFloat("specular_strength", 0.0f);


	//Load the textures for the cubemap
	GLuint cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

	// texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/night/";

	std::map<std::string, GLenum> facesToLoad = {
		{pathToCubeMap + "xneg.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "ypos.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "zpos_bis.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "xpos_bis.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "yneg.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "zneg_bis.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	//load the six faces
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}


	// Load texture for the ground
	GLuint texture_ground;
	glGenTextures(1, &texture_ground);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_ground);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_set_flip_vertically_on_load(true);
	int imWidth, imHeight, imNrChannels;
	char file[128] =  PATH_TO_TEXTURE "/asphalt.jpg";
	unsigned char* data_ground = stbi_load(file, &imWidth, &imHeight, &imNrChannels, 0);
	if (data_ground)
	{
		GLenum format = GL_RGB;
		if (imNrChannels == 1) format = GL_RED;
		else if (imNrChannels == 3) format = GL_RGB;
		else if (imNrChannels == 4) format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, imWidth, imHeight, 0, format, GL_UNSIGNED_BYTE, data_ground);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to Load texture of the ground" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}

	stbi_image_free(data_ground);

	//Load the texture for the walls;
	// Load texture for the ground
	GLuint texture_wall;
	glGenTextures(1, &texture_wall);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture_wall);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	int imWidthwall, imHeightwall, imNrChannelswall;
	char filewall[128] = PATH_TO_TEXTURE "/wall.jpg";
	unsigned char* data_wall= stbi_load(filewall, &imWidthwall, &imHeightwall, &imNrChannelswall, 0);
	if (data_wall)
	{
		GLenum formatwall = GL_RGB;
		if (imNrChannelswall == 1) formatwall = GL_RED;
		else if (imNrChannelswall == 3) formatwall = GL_RGB;
		else if (imNrChannelswall == 4) formatwall = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, formatwall, imWidthwall, imHeightwall, 0, formatwall, GL_UNSIGNED_BYTE, data_wall);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to Load texture WALL" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}

	stbi_image_free(data_wall);

	//Texture for the grass
	//Load the texture for the walls;
	// Load texture for the ground
	GLuint texture_grass;
	glGenTextures(1, &texture_grass);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture_grass);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	int imWidthgrass, imHeightgrass, imNrChannelsgrass;
	char filegrass[128] = PATH_TO_TEXTURE "/grass.jpg";
	unsigned char* data_grass = stbi_load(filegrass, &imWidthgrass, &imHeightgrass, &imNrChannelsgrass, 0);
	if (data_grass)
	{
		GLenum formatgrass = GL_RGB;
		if (imNrChannelsgrass == 1) formatgrass = GL_RED;
		else if (imNrChannelsgrass == 3) formatgrass = GL_RGB;
		else if (imNrChannelsgrass == 4) formatgrass = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, formatgrass, imWidthgrass, imHeightgrass, 0, formatgrass, GL_UNSIGNED_BYTE, data_grass);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to Load texture of the grass" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}

	stbi_image_free(data_grass);

	//Load the texture of the lamp
	GLuint texture_lamp;
	glGenTextures(1, &texture_lamp);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture_lamp);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	int imWidthlamp, imHeightlamp, imNrChannelslamp;
	char filelamp[128] = PATH_TO_TEXTURE "/lamp.png";
	unsigned char* data_lamp = stbi_load(filelamp, &imWidthlamp, &imHeightlamp, &imNrChannelslamp, 0);
	if (data_lamp)
	{
		GLenum formatlamp = GL_RGB;
		if (imNrChannelslamp == 1) formatlamp = GL_RED;
		else if (imNrChannelslamp == 3) formatlamp = GL_RGB;
		else if (imNrChannelslamp == 4) formatlamp = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, formatlamp, imWidthlamp, imHeightlamp, 0, formatlamp, GL_UNSIGNED_BYTE, data_lamp);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to Load texture LAMP" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}

	stbi_image_free(data_lamp);




	shader.use();
	shader.setInteger("cubemapSampler", 0);
	shader.setInteger("shadowMap", 4);
	cubeMapShader.use();
	cubeMapShader.setInteger("cubemapSampler", 0);
	wallShader.use();
	wallShader.setInteger("wallSampler", 2);
	groundShader.use();
	groundShader.setInteger("groundSampler", 1);
	groundShader.setInteger("shadowMap", 4);
	lampShader.use();
	lampShader.setInteger("lampSampler", 3);
	grassShader.use();
	grassShader.setInteger("grassSampler", 2);
	
	//initialise the particles
	const float vertexData[18] = {
		// vertices
		-1.0, -1.0, 0.0,
		1.0, -1.0, 0.0,
		-1.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		-1.0, 1.0, 0.0,
		1.0, -1.0, 0.0
	};


	static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	static GLfloat* g_particule_color_data = new GLfloat[MaxParticles * 4];

	for (int i = 0; i < MaxParticles; i++) {
		particlesContainer[i].life = -1.0;
	}

	//Create the vertex buffer objects for the quad used as a particle, 
	//and the positions and colors of all particle
	//the same vertex array object is used for all 3 VBOs
	GLuint VBO_vertex, VBO_position, VBO_color, VAO_particle;
	glGenVertexArrays(1, &VAO_particle);
	glGenBuffers(1, &VBO_vertex);
	glGenBuffers(1, &VBO_position);
	glGenBuffers(1, &VBO_color);

	//define VBO and VAO as active buffer and active vertex array
	glBindVertexArray(VAO_particle);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_vertex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	auto att_vertex = glGetAttribLocation(particleShader.ID, "vertex");
	glEnableVertexAttribArray(att_vertex);
	glVertexAttribPointer(att_vertex, 3, GL_FLOAT, false, 0, 0);
	glVertexAttribDivisor(att_vertex, 0);


	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);

	auto att_center = glGetAttribLocation(particleShader.ID, "center");
	glEnableVertexAttribArray(att_center);
	glVertexAttribPointer(att_center, 4, GL_FLOAT, false, 0, 0);
	glVertexAttribDivisor(att_center, 1);


	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	auto att_col = glGetAttribLocation(particleShader.ID, "col");
	glEnableVertexAttribArray(att_col);
	glVertexAttribPointer(att_col, 4, GL_FLOAT, true, 0, 0);
	glVertexAttribDivisor(att_col, 1);

	//desactive the buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	glm::vec3 cameraRight = camera.Right;
	glm::vec3 cameraUp = camera.Up;
	glm::vec3 cameraPosition = camera.Position;


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int window_width, window_height;
	glfwGetFramebufferSize(window, &window_width, &window_height);


	//COmpute the view of the moon for the shadow

	glm::vec3 moonPos = glm::vec3(1000.0f, 500.0f, -20.0f);
	glm::mat4 lightView = glm::lookAt(moonPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProj = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 1250.0f);
	glm::mat4 lightSpaceMatrix = lightProj * lightView;

	ShadowMap sm = createSM();
	const std::string sourceVshadow = readShaderFile(PATH_TO_VERT "/shadow.vert");
	const std::string sourceFshadow = readShaderFile(PATH_TO_FRAG "/shadow.frag");
	Shader shadowShader = Shader(sourceVshadow, sourceFshadow);


	glfwSwapInterval(1);
	double lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {



		glfwPollEvents();

		processInput(window, car_coord, carYaw); //Recupère les commandes de la voiture

		glViewport(0, 0, 1024, 1024);
		glBindFramebuffer(GL_FRAMEBUFFER, sm.FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadowShader.use();
		shadowShader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
		shadowShader.setMatrix4("M", wallMod);
		wall.draw();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		glViewport(0, 0, width, height);
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		


		

		glm::vec3 carFront = glm::vec3(sin(carYaw - glm::radians(90.0f)), 0.0f, cos(carYaw - glm::radians(90.0f)));
		if (isPaused == false) {
			speed = (speed + acc_std * acc_actu) * 0.98f;
			car_coord = car_coord + carFront * speed;
		}

		double now = glfwGetTime();
		//add the particles
		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		lastTime = currentTime;
		
		if (isPaused) {
			float cs = float(delta) * 100.0f;
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)camera.ProcessKeyboardMovement(FORWARD, cs);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)camera.ProcessKeyboardMovement(BACKWARD, cs);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)camera.ProcessKeyboardMovement(RIGHT, cs);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)camera.ProcessKeyboardMovement(LEFT, cs);

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)camera.ProcessKeyboardRotation(0.0, 1.0, 1.0);
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)camera.ProcessKeyboardRotation(0.0, -1.0, 1.0);
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)camera.ProcessKeyboardRotation(-1.0, 0.0, 1.0);
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)camera.ProcessKeyboardRotation(1.0, 0.0, 1.0);


		}
		//Active the texture of the cubemap
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		glDepthFunc(GL_LEQUAL);
		cubeMap.draw();
		glDepthFunc(GL_LESS);

		
		
		//Apply the vision on the car of the car
		model = glm::mat4(1.0);
		model = glm::translate(model, car_coord);
		model = glm::rotate(model, carYaw, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.1, 0.1, 0.07));
		glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
		inverseModel = glm::transpose(glm::inverse(model));
		glActiveTexture(GL_TEXTURE4); //ACTIV THE SHADOW ON THE CAR
		glBindTexture(GL_TEXTURE_2D, sm.texture); //ACTIV THE SHADOW ON THE CAR

		shader.use();

		shader.setMatrix4("M", model);
		shader.setMatrix4("itM", inverseModel);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setInteger("useRefraction", refract_actif ? 1 : 0);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix); //For the shadow
		car.draw();

		shader.setMatrix4("M", bunnyMod);
		shader.setMatrix4("itM", bunnyInv);
		shader.setVector3f("u_view_pos", camera.Position);
		shader.setInteger("useRefraction", refract_actif ? 1 : 0); //
		shader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix); //For the shadow
		bunny.draw();

		//Update the camera linked to the car
		if (isPaused == false) {
			camera.car_coord_update(car_coord, carYaw);
		}
		view = camera.GetViewMatrix(); // at this place because it has to be after the model update

		cameraRight = camera.Right;
		cameraUp = camera.Up;

		//The ground
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture_ground);

		groundShader.use();
		groundShader.setMatrix4("M", groundMod);
		groundShader.setMatrix4("V", view);
		groundShader.setMatrix4("P", perspective);
		groundShader.setMatrix4("itM", groundInv);
		groundShader.setMatrix4("lightSpaceMatrix", lightSpaceMatrix); // For the shadow

		ground.draw();
		groundShader.setVector3f("u_view_pos", camera.Position);

		//The grass
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture_grass);

		grassShader.use();
		grassShader.setVector3f("u_view_pos", camera.Position);

		grassShader.setMatrix4("M", grassMod);
		grassShader.setMatrix4("V", view);
		grassShader.setMatrix4("P", perspective);
		grassShader.setMatrix4("itM", grassInv);
		grass.draw();

		//The wall
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture_wall);

		wallShader.use();
		wallShader.setVector3f("u_view_pos", camera.Position);

		wallShader.setMatrix4("M", wallMod);
		wallShader.setMatrix4("V", view);
		wallShader.setMatrix4("P", perspective);
		wallShader.setMatrix4("itM", wallInv);
		wall.draw();

		//Active the lamp
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, texture_lamp);

		lampShader.use();
		lampShader.setVector3f("u_view_pos", camera.Position);

		lampShader.setMatrix4("V", view);
		lampShader.setMatrix4("P", perspective);
		for (const auto& lp : lampadaires) {
			lampShader.setMatrix4("M", lp.model);
			lampShader.setMatrix4("itM", lp.invModel);
			lamp.draw();
		}

		//Light bulb --> To delete
		lightShader.use();
		lightShader.setMatrix4("V", view);
		lightShader.setMatrix4("P", perspective);
		for (const auto& lp : lampadaires) {
			glm::mat4 sphereMod = glm::translate(glm::mat4(1.0f), lp.light_pos);
			sphereMod = glm::scale(sphereMod, glm::vec3(0.1f));
			lightShader.setMatrix4("M", sphereMod);
			lightShader.setVector3f("u_light_colour", lp.colour);
			lightSphere.draw();

		}

		//for the moon
		moonShader.use();
		moonShader.setMatrix4("V", view);
		moonShader.setMatrix4("P", perspective);
		glm::mat4 moonMod = glm::translate(glm::mat4(1.0f), moonPos);
		moonMod = glm::scale(moonMod, glm::vec3(7.0f));
		moonShader.setMatrix4("M", moonMod);
		moonShader.setVector3f("u_light_colour", glm::vec3(0.9f, 0.9f, 0.9f));
		moonSphere.draw();


		


		//Add new particles

		int newParticle = delta * 3000.0f;
		if (newParticle > (int)(0.032f * 1000.0)) newParticle = (int)(0.032f * 1000.0);

		if (isDrift == true)
			for (int i = 0; i < newParticle; i++) {
				int particleIdx = findUnusedParticle();
				particlesContainer[particleIdx].life = 3.0f + rand() % 10 / 20.0;

				float delta_time = float(i) / (float)newParticle;
				glm::vec3 position_start = glm::mix(car_coord_prev, car_coord, delta_time);
				particlesContainer[particleIdx].pos = position_start + glm::vec3((rand() % 100 - 50.0f) / 3000.0f, 0.0f, (rand() % 100 - 50.0) / 3000.0f);
				//particlesContainer[particleIdx].pos = car_coord + glm::vec3((rand() % 100 - 50.0f) / 3000.0f, 0.0f, (rand() % 100 - 50.0) / 3000.0f);  //glm::vec3(glm::cos(currentTime) * 0.5, 2.0 + rand() % 100 / 1000.0, -5.0f + rand() % 100 / 1000.0);
				glm::vec3 carPerpendicular = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(carFront, 0.0f));
				particlesContainer[particleIdx].speed = carPerpendicular * -driftDirection * ((rand() % 100 - 100.0f) /200.0f);

				//use hsv color to get pretty results
				particlesContainer[particleIdx].color = glm::vec4(15.0f, 1.0f, 1.0f, 0.8f);

				particlesContainer[particleIdx].size = 0.0075f;
			}

		//Simulate the particle
		int particleCount = 0;
		for (int i = 0; i < MaxParticles; i++) {
			Particle& p = particlesContainer[i]; //shortcut

			if (p.life > 0.0) {
				//decrease life, use time since last frame
				p.life -= delta;

				//change of pos
				p.pos += p.speed * (float)delta;

				p.color.r += (float)delta * 10.0f;
				p.color.g -= (float)delta * 1.0f;
				p.color.b -= (float)delta * 1.0f;
				p.color.a -= (float)delta * 0.5f;

				//update distance with the camera
				p.cameraDist = glm::length2(p.pos - camera.Position);

				//fill the gpu buffer
				g_particule_position_size_data[4 * particleCount] = p.pos.x;
				g_particule_position_size_data[4 * particleCount + 1] = p.pos.y;
				g_particule_position_size_data[4 * particleCount + 2] = p.pos.z;

				g_particule_position_size_data[4 * particleCount + 3] = p.size;

				glm::vec3 hsv = glm::vec3(p.color.r, p.color.g, p.color.b);
				glm::vec3 rgb = glm::rgbColor(hsv);
				g_particule_color_data[4 * particleCount + 0] = rgb.r;
				g_particule_color_data[4 * particleCount + 1] = rgb.g;
				g_particule_color_data[4 * particleCount + 2] = rgb.b;
				g_particule_color_data[4 * particleCount + 3] = p.color.a;

				particleCount++;
			}

			else {
				//make sure all dead particle will be put at the end of the list
				p.cameraDist = -1;
			}

		}

		sortParticles();



		glBindVertexArray(VAO_particle);
		glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);;
		glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

		glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);;
		glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 4, g_particule_color_data);


		particleShader.use();

		particleShader.setMatrix4("V", view);
		particleShader.setMatrix4("P", perspective);

		particleShader.setVector3f("cameraRight", cameraRight);
		particleShader.setVector3f("cameraUp", cameraUp);


		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, particleCount);


		car_coord_prev = car_coord;
		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


void loadCubemapFace(const char* path, const GLenum& targetFace)
{
	int imWidth, imHeight, imNrChannels;
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{
		GLenum formatcube = GL_RGB;
		if (imNrChannels == 1) formatcube = GL_RED;
		else if (imNrChannels == 3) formatcube = GL_RGB;
		else if (imNrChannels == 4) formatcube = GL_RGBA;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(targetFace, 0, formatcube, imWidth, imHeight, 0, formatcube, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(targetFace);
	}
	else {
		std::cout << "Failed to Load texture Cubemap" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}
	stbi_image_free(data);
}



void processInput(GLFWwindow* window, glm::vec3& car_coord, float& carYaw) {
	//3. Use the cameras class to change the parameters of the car and so the cmaera will be up to date
	acc_actu = 0.0f;
	isDrift = false;
	driftDirection = 0.0f;
	rot_direction = 1.0f;

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		if (R_press == false) {
			refract_actif = !refract_actif;
			R_press = true;
		}
	}

	else {
		R_press = false;

	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (p_press == false) {
			isPaused = !isPaused;
			p_press = true;
		}
	}

	else {
		p_press = false;

	}

	if (isPaused) { return; }

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)

		acc_actu = 0.5f;
	//car_coord = car_coord+carFront*speed;
	
	
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		acc_actu = -0.8f;
		rot_direction = -1.0f;
		//car_coord = car_coord - carFront*speed;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		isDrift = true;
		carYaw = carYaw + rot_direction * rotspeed;
		driftDirection = -1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		isDrift = true;
		carYaw = carYaw - rot_direction*rotspeed;
		driftDirection = 1.0f;
	}

	

	//if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	//	camera.ProcessKeyboardRotation(1, 0.0, 1);
	//if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	//	camera.ProcessKeyboardRotation(-1, 0.0, 1);
	//
	//if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	//	camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	//if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	//	camera.ProcessKeyboardRotation(0.0, -1.0, 1);


}

