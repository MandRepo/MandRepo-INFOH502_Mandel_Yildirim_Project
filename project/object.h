#ifndef OBJECT_H
#define OBJECT_H

#include<iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>



struct Vertex {
	glm::vec3 Position;
	glm::vec2 Texture;
	glm::vec3 Normal;
};


class Object
{
public:
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textures;
	std::vector<glm::vec3> normals;
	std::vector<Vertex> vertices;

	int numVertices;

	GLuint VBO, VAO;

	glm::mat4 model = glm::mat4(1.0);


	// Parse one face token like "12", "12/34", "12//56", "12/34/56"
	// Returns a Vertex, gracefully handling missing texture or normal indices.
	Vertex parseFaceToken(const std::string& token) {
		Vertex v;
		v.Position = glm::vec3(0.0f);
		v.Texture  = glm::vec2(0.0f);
		v.Normal   = glm::vec3(0.0f);

		// Split on '/'
		std::string p, t, n;
		size_t first = token.find('/');
		if (first == std::string::npos) {
			// Format: "v"
			p = token;
		} else {
			p = token.substr(0, first);
			size_t second = token.find('/', first + 1);
			if (second == std::string::npos) {
				// Format: "v/t"
				t = token.substr(first + 1);
			} else {
				// Format: "v/t/n" or "v//n"
				t = token.substr(first + 1, second - first - 1);
				n = token.substr(second + 1);
			}
		}

		// Position is mandatory
		if (!p.empty()) {
			int pi = std::stoi(p);
			if (pi < 0) pi = (int)positions.size() + pi + 1; // negative indices = from end
			if (pi >= 1 && pi <= (int)positions.size())
				v.Position = positions[pi - 1];
		}
		// Texture is optional
		if (!t.empty() && !textures.empty()) {
			int ti = std::stoi(t);
			if (ti < 0) ti = (int)textures.size() + ti + 1;
			if (ti >= 1 && ti <= (int)textures.size())
				v.Texture = textures[ti - 1];
		}
		// Normal is optional
		if (!n.empty() && !normals.empty()) {
			int ni = std::stoi(n);
			if (ni < 0) ni = (int)normals.size() + ni + 1;
			if (ni >= 1 && ni <= (int)normals.size())
				v.Normal = normals[ni - 1];
		}
		return v;
	}

	Object(const char* path) {

		std::ifstream infile(path);
		if (!infile.is_open()) {
			std::cerr << "ERROR: Could not open OBJ file: " << path << std::endl;
			numVertices = 0;
			return;
		}
		std::string line;
		while (std::getline(infile, line))
		{
			std::istringstream iss(line);
			std::string indice;
			iss >> indice;
			//std::cout << "indice : " << indice << std::endl;
			if (indice == "v") {
				float x, y, z;
				iss >> x >> y >> z;
				positions.push_back(glm::vec3(x, y, z));

			}
			else if (indice == "vn") {
				float x, y, z;
				iss >> x >> y >> z;
				normals.push_back(glm::vec3(x, y, z));
			}
			else if (indice == "vt") {
				float u, v;
				iss >> u >> v;
				textures.push_back(glm::vec2(u, v));
			}
			else if (indice == "f") {
				// Read all face tokens (3, 4, or more vertices per face)
				std::vector<std::string> faceTokens;
				std::string tok;
				while (iss >> tok) faceTokens.push_back(tok);

				if (faceTokens.size() < 3) continue; // malformed face

				// Fan triangulation: for face (v0, v1, v2, v3, ...),
				// emit triangles (v0,v1,v2), (v0,v2,v3), (v0,v3,v4), ...
				Vertex v0 = parseFaceToken(faceTokens[0]);
				for (size_t i = 1; i + 1 < faceTokens.size(); ++i) {
					Vertex vi = parseFaceToken(faceTokens[i]);
					Vertex vj = parseFaceToken(faceTokens[i + 1]);
					vertices.push_back(v0);
					vertices.push_back(vi);
					vertices.push_back(vj);
				}
			}
		}
		//std::cout << positions.size() << std::endl;
		//std::cout << normals.size() << std::endl;
		//std::cout << textures.size() << std::endl;
		std::cout << "Load model with " << vertices.size() << " vertices" << std::endl;

		infile.close();

		numVertices = vertices.size();
	}



	void makeObject(Shader shader, bool texture = true) {
		/* This is a working but not perfect solution, you can improve it if you need/want
		* What happens if you call this function twice on an Model ?
		* What happens when a shader doesn't have a position, tex_coord or normal attribute ?
		*/

		float* data = new float[8 * numVertices];
		for (int i = 0; i < numVertices; i++) {
			Vertex v = vertices.at(i);
			data[i * 8] = v.Position.x;
			data[i * 8 + 1] = v.Position.y;
			data[i * 8 + 2] = v.Position.z;

			data[i * 8 + 3] = v.Texture.x;
			data[i * 8 + 4] = v.Texture.y;

			data[i * 8 + 5] = v.Normal.x;
			data[i * 8 + 6] = v.Normal.y;
			data[i * 8 + 7] = v.Normal.z;
		}

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		//define VBO and VAO as active buffer and active vertex array
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, data, GL_STATIC_DRAW);

		auto att_pos = glGetAttribLocation(shader.ID, "position");
		glEnableVertexAttribArray(att_pos);
		glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);


		if (texture) {
			auto att_tex = glGetAttribLocation(shader.ID, "tex_coords");
			glEnableVertexAttribArray(att_tex);
			glVertexAttribPointer(att_tex, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));

		}

		auto att_col = glGetAttribLocation(shader.ID, "normal");
		glEnableVertexAttribArray(att_col);
		glVertexAttribPointer(att_col, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(5 * sizeof(float)));

		//desactive the buffer
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		delete[] data;

	}

	void draw() {

		glBindVertexArray(this->VAO);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

	}
};
#endif