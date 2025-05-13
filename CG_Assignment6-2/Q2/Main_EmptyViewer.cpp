// 리팩토링된 Gouraud Shading 렌더링 코드 (GLM 방식 중심)
#include <Windows.h>
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Object.h"

using namespace glm;

int Width = 512;
int Height = 512;

std::vector<vec3> gVertices;
std::vector<int> gIndexBuffer;
std::vector<float> DepthBuffer(Width* Height, 1e9f);
std::vector<float> OutputImage(Width* Height * 3, 0.0f);

void create_scene() {
    int Width = 32, Height = 16;
    gVertices.resize((Height - 2) * Width + 2);
    gIndexBuffer.resize((Height - 2) * (Width - 1) * 6 + 6 * (Width - 1));

    int t = 0;
    for (int j = 1; j < Height - 1; ++j) {
        for (int i = 0; i < Width; ++i) {
            float theta = float(j) / (Height - 1) * pi<float>();
            float phi = float(i) / (Width - 1) * 2 * pi<float>();
            float x = sin(theta) * cos(phi);
            float y = cos(theta);
            float z = -sin(theta) * sin(phi);
            gVertices[t++] = vec3(x, y, z);
        }
    }
    gVertices[t++] = vec3(0, 1, 0);
    gVertices[t++] = vec3(0, -1, 0);

    t = 0;
    for (int j = 0; j < Height - 3; ++j) {
        for (int i = 0; i < Width - 1; ++i) {
            gIndexBuffer[t++] = j * Width + i;
            gIndexBuffer[t++] = (j + 1) * Width + (i + 1);
            gIndexBuffer[t++] = j * Width + (i + 1);
            gIndexBuffer[t++] = j * Width + i;
            gIndexBuffer[t++] = (j + 1) * Width + i;
            gIndexBuffer[t++] = (j + 1) * Width + (i + 1);
        }
    }
    for (int i = 0; i < Width - 1; ++i) {
        gIndexBuffer[t++] = (Height - 2) * Width;
        gIndexBuffer[t++] = i;
        gIndexBuffer[t++] = i + 1;
        gIndexBuffer[t++] = (Height - 2) * Width + 1;
        gIndexBuffer[t++] = (Height - 3) * Width + (i + 1);
        gIndexBuffer[t++] = (Height - 3) * Width + i;
    }
}

vec3 phong_color(vec3 pos, vec3 normal) {
    vec3 ka(0, 1, 0), kd(0, 0.5, 0), ks(0.5);
    vec3 light_pos(-4, 3, -3), view_pos(0);
    vec3 l = normalize(light_pos - pos), v = normalize(view_pos - pos), h = normalize(l + v);
    vec3 ambient = 0.2f * ka;
    vec3 diffuse = kd * max(dot(normal, l), 0.0f); 
    vec3 specular = ks * static_cast<float>(pow(max(dot(normal, h), 0.0f), 32.0f));
    vec3 color = ambient + diffuse + specular;
    return pow(clamp(color, 0.0f, 1.0f), vec3(1.0f / 2.2f));
}

void rasterize_triangle(vec4 v0, vec4 v1, vec4 v2, vec3 c0, vec3 c1, vec3 c2) {
    vec3 p0 = vec3(v0) / v0.w, p1 = vec3(v1) / v1.w, p2 = vec3(v2) / v2.w;
    auto to_screen = [](vec3 p) { return vec2((p.x + 1) * 0.5f * Width, (p.y + 1) * 0.5f * Height); };
    vec2 s0 = to_screen(p0), s1 = to_screen(p1), s2 = to_screen(p2);

    int minX = floor(std::min({ s0.x, s1.x, s2.x })), maxX = ceil(std::max({ s0.x, s1.x, s2.x }));
    int minY = floor(std::min({ s0.y, s1.y, s2.y })), maxY = ceil(std::max({ s0.y, s1.y, s2.y }));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            vec2 p(x + 0.5f, y + 0.5f);
            vec2 e0 = s1 - s0, e1 = s2 - s1, e2 = s0 - s2;
            vec2 vp0 = p - s0, vp1 = p - s1, vp2 = p - s2;

            float a = e0.x * vp0.y - e0.y * vp0.x;
            float b = e1.x * vp1.y - e1.y * vp1.x;
            float c = e2.x * vp2.y - e2.y * vp2.x;
            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0)) {
                float area = abs((s1.x - s0.x) * (s2.y - s0.y) - (s2.x - s0.x) * (s1.y - s0.y));
                float w0 = abs((s1.x - p.x) * (s2.y - p.y) - (s2.x - p.x) * (s1.y - p.y)) / area;
                float w1 = abs((s2.x - p.x) * (s0.y - p.y) - (s0.x - p.x) * (s2.y - p.y)) / area;
                float w2 = 1.0f - w0 - w1;
                float z = w0 * p0.z + w1 * p1.z + w2 * p2.z;

                int idx = y * Width + x;
                if (idx < 0 || idx >= Width * Height) continue;
                if (z < DepthBuffer[idx]) {
                    DepthBuffer[idx] = z;
                    vec3 color = w0 * c0 + w1 * c1 + w2 * c2;
                    OutputImage[3 * idx + 0] = color.r;
                    OutputImage[3 * idx + 1] = color.g;
                    OutputImage[3 * idx + 2] = color.b;
                }
            }
        }
    }
}

void render() {
    create_scene();
    mat4 model = scale(translate(mat4(1), vec3(0, 0, -7)), vec3(2));
    mat4 view = lookAt(vec3(0), vec3(0, 0, -1), vec3(0, 1, 0));
    mat4 proj = frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 100.0f);
    mat4 MVP = proj * view * model;

    std::vector<vec3> colors(gVertices.size());
    for (int i = 0; i < gVertices.size(); ++i) {
        vec3 pos = vec3(model * vec4(gVertices[i], 1));
        vec3 norm = normalize(mat3(transpose(inverse(model))) * gVertices[i]);
        colors[i] = phong_color(pos, norm);
    }

    for (int i = 0; i < gIndexBuffer.size(); i += 3) {
        int ia = gIndexBuffer[i], ib = gIndexBuffer[i + 1], ic = gIndexBuffer[i + 2];
        rasterize_triangle(MVP * vec4(gVertices[ia], 1), MVP * vec4(gVertices[ib], 1), MVP * vec4(gVertices[ic], 1),
            colors[ia], colors[ib], colors[ic]);
    }
}


void resize_callback(GLFWwindow*, int nw, int nh)
{
    //This is called in response to the window resizing.
    //The new Width and Height are passed in so we make 
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
