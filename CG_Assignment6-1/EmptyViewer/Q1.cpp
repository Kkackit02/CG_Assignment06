﻿#include <Windows.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "sphere_scene.h"
#include "Object.h"
#include "Material.h"
#include "viewPipeline.h"
using namespace glm;

inline glm::vec3 make_vec3(const float arr[3]) {
    return glm::vec3(arr[0], arr[1], arr[2]);
}

int Width = 512;
int Height = 512;
std::vector<float> OutputImage; 

inline glm::vec3 calculate_flat_shading_color(
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    const glm::vec3& light_pos,
    const glm::vec3& view_pos,
    const glm::vec3& kd,
    const glm::vec3& ka,
    const glm::vec3& ks,
    float p,
    const glm::vec3& light_color,
    const glm::vec3& la)
{
    glm::vec3 N = glm::normalize(glm::cross(v1 - v0, v2 - v0)); 
    glm::vec3 P = (v0 + v1 + v2) / 3.0f;                       
    glm::vec3 L = glm::normalize(light_pos - P);               
    glm::vec3 V = glm::normalize(view_pos - P);                
    glm::vec3 H = glm::normalize(V + L);                       

    float diff = glm::max(glm::dot(N, L), 0.0f);
    
	float spec = glm::pow(glm::max(glm::dot(N, H), 0.0f), p); // Blinn-Phong

    glm::vec3 ambient = ka * la;
    glm::vec3 diffuse = kd * diff * light_color;
    glm::vec3 specular = ks * spec * light_color;
    return glm::clamp(ambient + diffuse + specular, 0.0f, 1.0f);
}



void rasterize_triangle(
    std::vector<float>& framebuffer,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    const glm::vec3& color,
    float** z_buffer,
    int width, int height) {

    int xmin = std::max(0, (int)std::floor(std::min({ v0.x, v1.x, v2.x })));
    int xmax = std::min(width - 1, (int)std::ceil(std::max({ v0.x, v1.x, v2.x })));
    int ymin = std::max(0, (int)std::floor(std::min({ v0.y, v1.y, v2.y })));
    int ymax = std::min(height - 1, (int)std::ceil(std::max({ v0.y, v1.y, v2.y })));

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            glm::vec2 p = glm::vec2(x + 0.5f, y + 0.5f);
            glm::vec2 a = glm::vec2(v0);
            glm::vec2 b = glm::vec2(v1);
            glm::vec2 c = glm::vec2(v2);

            float denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            float beta = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / denom;
            float gamma = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / denom;
            float alpha = 1.0f - beta - gamma;

            if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                float z = alpha * v0.z + beta * v1.z + gamma * v2.z;
                if (z < z_buffer[y][x]) {
                    z_buffer[y][x] = z;
                    int screen_x = width - 1 - x;
                    int screen_y = height - 1 - y;  // Y축 반전
                    int idx = (screen_y * width + screen_x) * 3;
                    framebuffer[idx + 0] = color.r;
                    framebuffer[idx + 1] = color.g;
                    framebuffer[idx + 2] = color.b;
                }
            }
        }
    }
}

void render() {
    OutputImage.clear();
    OutputImage.resize(Width * Height * 3, 0.0f);

    ObjectData data = create_scene(32, 16);
    float model[4][4] = {
        {2, 0, 0, 0},
        {0, 2, 0, 0},
        {0, 0, 2, -7},
        {0, 0, 0, 1}
    };

    Camera camera = { {0,0,0}, {1,0,0}, {0,1,0}, {0,0,-1} };
    glm::vec3 view_pos = make_vec3(camera.eye);

    glm::vec3 LightSource = { -4, 4, -3 };
    glm::vec4 light4 = glm::vec4(LightSource, 1.0f);
   

    Material M = { {0,1,0}, {0,0.5f,0}, {0.5f,0.5f,0.5f}, 32.0f, glm::vec3(0.2f) };
    float gammaCorrection = 2.2f;

    std::vector<glm::vec3> light_vertices;
    std::vector<glm::vec3> screen_vertices;
    light_vertices.reserve(data.numVertices);
    screen_vertices.reserve(data.numVertices);
    float** z_buffer = new float* [Height];


    for (int i = 0; i < Height; i++) {
        z_buffer[i] = new float[Width];
        std::fill(z_buffer[i], z_buffer[i] + Width, 1.0f);
    }

    // 정점 처리
    for (int i = 0; i < data.numVertices; i++) {
        float vert4[4] = {
            data.vertexBuffer[3 * i + 0],
            data.vertexBuffer[3 * i + 1],
            data.vertexBuffer[3 * i + 2],
            1.0f
        };
        float* mv = M_Model(vert4, model);
        
        light_vertices.emplace_back(mv[0] / mv[3], mv[1] / mv[3], mv[2] / mv[3]);
        float* cv = M_Camera(mv, camera);
        float* pv = M_Perspective(cv, -0.1f, -1000);
        float* ov = M_Orthograph(pv, -0.1f, 0.1f, -0.1f, 0.1f, -0.1f, -1000);
        float* sv = M_Viewport(ov, Width, Height);
        screen_vertices.emplace_back(sv[0] / sv[3], sv[1] / sv[3], sv[2] / sv[3]);

    }

    std::vector<glm::vec3> triangle_colors(data.numTriangles);
    for (int i = 0; i < data.numTriangles; i++) {
        int k0 = data.indexBuffer[3 * i + 0];
        int k1 = data.indexBuffer[3 * i + 1];
        int k2 = data.indexBuffer[3 * i + 2];

       
        if (k0 < 0 || k1 < 0 || k2 < 0 || k0 >= data.numVertices || k1 >= data.numVertices || k2 >= data.numVertices)
            continue;
         triangle_colors[i] = calculate_flat_shading_color(
            light_vertices[k0], light_vertices[k1], light_vertices[k2],
            LightSource,
            view_pos,
            M.kd, M.ka, M.ks, M.p,
            glm::vec3(1.0f), M.Ia
        );  

    }

    // 래스터라이징
    for (int i = 0; i < data.numTriangles; i++) {
        int k0 = data.indexBuffer[3 * i + 0];
        int k1 = data.indexBuffer[3 * i + 1];
        int k2 = data.indexBuffer[3 * i + 2];
        rasterize_triangle(OutputImage,
            screen_vertices[k0], screen_vertices[k1], screen_vertices[k2],
            triangle_colors[i], z_buffer, Width, Height);
    }

    // 감마 보정
    for (auto& v : OutputImage) {
        v = glm::clamp(powf(v, 1.0f / gammaCorrection), 0.0f, 1.0f);
    }
    // Z-buffer 해제
    for (int i = 0; i < Height; i++) delete[] z_buffer[i];
    delete[] z_buffer;
}
void resize_callback(GLFWwindow*, int nw, int nh)
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
