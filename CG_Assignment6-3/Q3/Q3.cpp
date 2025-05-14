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
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace std;
using namespace glm;

int Width = 512;
int Height = 512;
vector<float> OutputImage;

struct Vertex {
    vec3 pos, normal, screen;
};

vector<Vertex> gVertices;
vector<int> gIndexBuffer;
vector<float> gDepthBuffer;
int gNumVertices = 0, gNumTriangles = 0;
static constexpr float PI = 3.14159265358979323846f;

void create_scene() {
    const int slices = 32, stacks = 16;
    gNumVertices = (stacks - 2) * slices + 2;
    gNumTriangles = (stacks - 3) * (slices - 1) * 2 + 2 * (slices - 1);
    gVertices.resize(gNumVertices);
    gIndexBuffer.resize(3 * gNumTriangles);
    int t = 0;
    for (int j = 1; j < stacks - 1; ++j) {
        float theta = float(j) / float(stacks - 1) * PI;
        for (int i = 0; i < slices; ++i) {
            float phi = float(i) / float(slices - 1) * 2.0f * PI;
            vec3 p = {
                sin(theta) * cos(phi),
                cos(theta),
                -sin(theta) * sin(phi)
            };
            gVertices[t].pos = p;
            gVertices[t].normal = normalize(p);
            ++t;
        }
    }
    gVertices[t++] = { vec3(0, 1, 0), vec3(0, 1, 0) };
    gVertices[t++] = { vec3(0, -1, 0), vec3(0, -1, 0) };
    int idx = 0;
    for (int j = 0; j < stacks - 3; ++j) {
        for (int i = 0; i < slices - 1; ++i) {
            int a = j * slices + i;
            int b = (j + 1) * slices + (i + 1);
            int c = j * slices + (i + 1);
            int d = (j + 1) * slices + i;
            gIndexBuffer[idx++] = a; gIndexBuffer[idx++] = b; gIndexBuffer[idx++] = c;
            gIndexBuffer[idx++] = a; gIndexBuffer[idx++] = d; gIndexBuffer[idx++] = b;
        }
    }
    int north = gNumVertices - 2, south = gNumVertices - 1;
    for (int i = 0; i < slices - 1; ++i) {
        gIndexBuffer[idx++] = north; gIndexBuffer[idx++] = i; gIndexBuffer[idx++] = i + 1;
        gIndexBuffer[idx++] = south;
        gIndexBuffer[idx++] = (stacks - 3) * slices + (i + 1);
        gIndexBuffer[idx++] = (stacks - 3) * slices + i;
    }
}

struct Camera {
    vec3 eye, u, v, w;
};

Camera camera = {
    vec3(0, 0, 0),
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, -1)
};

void render() {
    create_scene();
    OutputImage.assign(Width * Height * 3, 0.0f);
    gDepthBuffer.assign(Width * Height, numeric_limits<float>::infinity());

    // 모델 변환 
    mat4 M_model = translate(mat4(1.0f), vec3(0, 0, -7)) * scale(mat4(1.0f), vec3(2.0f));

    // 카메라 변환
    glm::mat4 M_cam = glm::mat4{
    glm::vec4(camera.u, 0.0f),
    glm::vec4(camera.v, 0.0f),
    glm::vec4(camera.w, 0.0f),
    glm::vec4(
        -glm::dot(camera.u, camera.eye),
        -glm::dot(camera.v, camera.eye),
        -glm::dot(camera.w, camera.eye),
        1.0f
    )
    };
    // perspective 
    float n = -0.1f, f = -1000.0f;
    mat4 M_persp = mat4{
        vec4(n,    0.0f, 0.0f, 0.0f),
        vec4(0.0f, n,    0.0f, 0.0f),
        vec4(0.0f, 0.0f, n + f, 1.0f),
        vec4(0.0f, 0.0f, -n * f, 0.0f)
    };

    // Orthographic 
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f;
    mat4 M_ortho = mat4{
        vec4(2.0f / (r - l), 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 2.0f / (t - b), 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 2.0f / (n - f), 0.0f),
        vec4(-(r + l) / (r - l), -(t + b) / (t - b), -(n + f) / (n - f), 1.0f)
    };

    // Viewport 
    float nx = float(Width), ny = float(Height);
    mat4 M_view = mat4{
    vec4(-nx * 0.5f,  0.0f,         0.0f, 0.0f),    // x축 반전
    vec4(0.0f,       -ny * 0.5f,    0.0f, 0.0f),    // y축 반전
    vec4(0.0f,        0.0f,         1.0f, 0.0f),
    vec4((nx - 1.0f) * 0.5f, (ny - 1.0f) * 0.5f, 0.0f, 1.0f)
    };
    mat4 M = M_view * M_ortho * M_persp * M_cam * M_model;

    for (auto& v : gVertices) {
        vec4 p = M * vec4(v.pos, 1.0f);
        if (p.w != 0.0f) p /= p.w;
        v.screen = vec3(p);
    }

    const vec3 ka{ 0, 1, 0 }, kd{ 0, 0.5f, 0 }, ks{ 0.5f, 0.5f, 0.5f }, lightPos{ -4, 4, -3 };
    const float shininess = 32.0f, Ia = 0.2f;
    mat3 normal_matrix = transpose(inverse(mat3(M_model)));

    for (int tri = 0; tri < gNumTriangles; ++tri) {
        int i0 = gIndexBuffer[3 * tri], i1 = gIndexBuffer[3 * tri + 1], i2 = gIndexBuffer[3 * tri + 2];
        const auto& V0 = gVertices[i0], & V1 = gVertices[i1], & V2 = gVertices[i2];

        vec3 P0 = vec3(M_model * vec4(V0.pos, 1));
        vec3 P1 = vec3(M_model * vec4(V1.pos, 1));
        vec3 P2 = vec3(M_model * vec4(V2.pos, 1));
        vec3 N0 = normalize(normal_matrix * V0.normal);
        vec3 N1 = normalize(normal_matrix * V1.normal);
        vec3 N2 = normalize(normal_matrix * V2.normal);

        int xmin = std::max(0, int(floor(min({ V0.screen.x, V1.screen.x, V2.screen.x }))));
        int xmax = std::min(Width - 1, int(ceil(max({ V0.screen.x, V1.screen.x, V2.screen.x }))));
        int ymin = std::max(0, int(floor(min({ V0.screen.y, V1.screen.y, V2.screen.y }))));
        int ymax = std::min(Height - 1, int(ceil(max({ V0.screen.y, V1.screen.y, V2.screen.y }))));

        for (int y = ymin; y <= ymax; ++y) 
        {
            for (int x = xmin; x <= xmax; ++x) 
            {
                vec2 p(x + 0.5f, y + 0.5f);
                vec2 a(V0.screen.x, V0.screen.y);
                vec2 b(V1.screen.x, V1.screen.y);
                vec2 c(V2.screen.x, V2.screen.y);
                float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                float w0 = ((b.x - p.x) * (c.y - p.y) - (b.y - p.y) * (c.x - p.x)) / area;
                float w1 = ((c.x - p.x) * (a.y - p.y) - (c.y - p.y) * (a.x - p.x)) / area;
                float w2 = 1.0f - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;

                float z = w0 * V0.screen.z + w1 * V1.screen.z + w2 * V2.screen.z;
                int idx = y * Width + x;

                if (z < gDepthBuffer[idx]) 
                {
                    gDepthBuffer[idx] = z;
                    vec3 P = w0 * P0 + w1 * P1 + w2 * P2;
                    vec3 N = normalize(w0 * N0 + w1 * N1 + w2 * N2);
                    vec3 L = normalize(lightPos - P);
                    vec3 V = normalize(-P);
                    vec3 H = normalize(L + V);  // Blinn-Phong 핵심

                    vec3 ambient = Ia * ka;
                    vec3 diffuse = std::max(dot(N, L), 0.0f) * kd;
                    vec3 specular = pow(std::max(dot(N, H), 0.0f), shininess) * ks;

                    vec3 color = clamp(ambient + diffuse + specular, 0.0f, 1.0f);
                    color = glm::pow(color, glm::vec3(1.0f / 2.2f));  // 감마 보정

                    OutputImage[3 * idx + 0] = color.r;
                    OutputImage[3 * idx + 1] = color.g;
                    OutputImage[3 * idx + 2] = color.b;
                }
            }
        }
    }
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
