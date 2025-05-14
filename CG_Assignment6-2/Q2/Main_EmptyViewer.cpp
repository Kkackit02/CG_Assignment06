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

#include "sphere_scene.h"
#include "Object.h"
#include "Material.h"
#include "viewPipeline.h"
using namespace glm;

inline glm::vec3 make_vec3(const float arr[3]) {
    return glm::vec3(arr[0], arr[1], arr[2]);
}
struct Vertex {
    glm::vec3 screen_pos;  // 화면 공간 위치 (x, y, z)
    float w;               // 투영된 w값 (perspective 보정용)
    glm::vec3 color;       // 조명 계산된 색상
};
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;

#define M_PI 3.14159265358979323846
// Gouraud Shading Rasterizer


ObjectData create_scene(int width, int height) {
    ObjectData data;

    float theta, phi;
    int t;

    data.numVertices = (height - 2) * width + 2; // 중간 + 위/아래 극점
    data.numTriangles = (height - 3) * width * 2 + width * 2; // 중간 띠 + 위/아래 극점 삼각형

    data.vertexBuffer = new float[3 * data.numVertices];
    data.indexBuffer = new int[3 * data.numTriangles];

    // 정점 생성
    t = 0;
    for (int j = 1; j < height - 1; ++j) {
        for (int i = 0; i < width; ++i) {
            theta = (float)j / (height - 1) * M_PI;
            phi = (float)i / (width) * 2 * M_PI; // width로 나눔으로써 wrap-around 가능

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);

            data.vertexBuffer[3 * t + 0] = x;
            data.vertexBuffer[3 * t + 1] = y;
            data.vertexBuffer[3 * t + 2] = z;
            ++t;
        }
    }

    // 북극 정점
    data.vertexBuffer[3 * t + 0] = 0;
    data.vertexBuffer[3 * t + 1] = 1;
    data.vertexBuffer[3 * t + 2] = 0;
    ++t;

    // 남극 정점
    data.vertexBuffer[3 * t + 0] = 0;
    data.vertexBuffer[3 * t + 1] = -1;
    data.vertexBuffer[3 * t + 2] = 0;
    ++t;

    // 삼각형 인덱스: 중간 띠들
    t = 0;
    for (int j = 0; j < height - 3; ++j) {
        for (int i = 0; i < width; ++i) {
            int i_next = (i + 1) % width;

            int a = j * width + i;
            int b = (j + 1) * width + i;
            int c = j * width + i_next;
            int d = (j + 1) * width + i_next;

            // 위 삼각형
            data.indexBuffer[t++] = a;
            data.indexBuffer[t++] = d;
            data.indexBuffer[t++] = c;

            // 아래 삼각형
            data.indexBuffer[t++] = a;
            data.indexBuffer[t++] = b;
            data.indexBuffer[t++] = d;
        }
    }

    // 북극 삼각형
    int north_idx = (height - 2) * width;
    for (int i = 0; i < width; ++i) {
        int i_next = (i + 1) % width;
        data.indexBuffer[t++] = north_idx;
        data.indexBuffer[t++] = i;
        data.indexBuffer[t++] = i_next;
    }

    // 남극 삼각형
    int south_idx = north_idx + 1;
    for (int i = 0; i < width; ++i) {
        int i_next = (i + 1) % width;
        data.indexBuffer[t++] = south_idx;
        data.indexBuffer[t++] = (height - 3) * width + i_next;
        data.indexBuffer[t++] = (height - 3) * width + i;
    }

    return data;
}


inline glm::vec3 calculate_vertex_color(
    const glm::vec3& pos,
    const glm::vec3& normal,
    const glm::vec3& light_pos,
    const glm::vec3& view_pos,
    const glm::vec3& kd,
    const glm::vec3& ka,
    const glm::vec3& ks,
    float p,
    const glm::vec3& light_color,
    const glm::vec3& la)
{
    glm::vec3 L = glm::normalize(light_pos - pos);
    glm::vec3 V = glm::normalize(view_pos - pos);
    glm::vec3 H = glm::normalize(V + L);
    glm::vec3 N = glm::normalize(normal);

    float diff = glm::max(glm::dot(N, L), 0.0f);
    float spec = glm::pow(glm::max(glm::dot(N, H), 0.0f), p);

    glm::vec3 ambient = ka * la;
    glm::vec3 diffuse = kd * diff * light_color;
    glm::vec3 specular = ks * spec * light_color;

    vec3 color = ambient + diffuse + specular;


    return clamp(color, 0.0f, 1.0f);

}void rasterize_triangle_gouraud_perspective(
    std::vector<float>& framebuffer,
    const Vertex& v0, const Vertex& v1, const Vertex& v2,
    float** z_buffer,
    int width, int height
) {
    glm::vec2 a = glm::vec2(v0.screen_pos);
    glm::vec2 b = glm::vec2(v1.screen_pos);
    glm::vec2 c = glm::vec2(v2.screen_pos);

    int xmin = std::max(0, (int)std::floor(std::min({ a.x, b.x, c.x })));
    int xmax = std::min(width - 1, (int)std::ceil(std::max({ a.x, b.x, c.x })));
    int ymin = std::max(0, (int)std::floor(std::min({ a.y, b.y, c.y })));
    int ymax = std::min(height - 1, (int)std::ceil(std::max({ a.y, b.y, c.y })));

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            int screen_y = height - 1 - y;
            int screen_x = width - 1 - x;

            glm::vec2 p(x + 0.5f, y + 0.5f);

            float denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            if (std::abs(denom) < 1e-5f) continue;

            float beta = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / denom;
            float gamma = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / denom;
            float alpha = 1.0f - beta - gamma;

            if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                float invW0 = 1.0f / v0.w;
                float invW1 = 1.0f / v1.w;
                float invW2 = 1.0f / v2.w;

                float w_inv_interp = alpha * invW0 + beta * invW1 + gamma * invW2;
                float z_numerator = alpha * v0.screen_pos.z * invW0 +
                    beta * v1.screen_pos.z * invW1 +
                    gamma * v2.screen_pos.z * invW2;
                float z = z_numerator / w_inv_interp;

                if (z < z_buffer[screen_y][screen_x]) {
                    z_buffer[screen_y][screen_x] = z;

                    glm::vec3 c0w = v0.color * invW0;
                    glm::vec3 c1w = v1.color * invW1;
                    glm::vec3 c2w = v2.color * invW2;

                    glm::vec3 color_numer = alpha * c0w + beta * c1w + gamma * c2w;
                    glm::vec3 final_color = color_numer / w_inv_interp;

                    final_color.r = pow(std::max(0.0f, final_color.r), 1.0f / 2.2f);
                    final_color.g = pow(std::max(0.0f, final_color.g), 1.0f / 2.2f);
                    final_color.b = pow(std::max(0.0f, final_color.b), 1.0f / 2.2f);

                    int idx = (screen_y * width + screen_x) * 3;
                    framebuffer[idx + 0] = final_color.r;
                    framebuffer[idx + 1] = final_color.g;
                    framebuffer[idx + 2] = final_color.b;
                    if (screen_x > width / 2 - 5 && screen_x < width / 2 + 5 &&
                        screen_y > height / 2 - 5 && screen_y < height / 2 + 5) {
                        std::cout << "Pixel (" << screen_x << ", " << screen_y << ") -> "
                            << "Color: (" << final_color.r << ", " << final_color.g << ", " << final_color.b << ")"
                            << ", Z: " << z << std::endl;
                    }

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
    Material M = { {0,1,0}, {0,0.5f,0}, {0.5f,0.5f,0.5f}, 32.0f, glm::vec3(0.2f) };
    float gammaCorrection = 2.2f;

    std::vector<glm::vec3> light_vertices(data.numVertices);
    std::vector<glm::vec3> screen_vertices(data.numVertices);
    std::vector<glm::vec3> vertex_colors(data.numVertices);
    std::vector<glm::vec3> vertex_normal(data.numVertices);
    std::vector<glm::vec3> world_positions(data.numVertices);
    std::vector<float> w_values(data.numVertices);

    float** z_buffer = new float* [Height];
    for (int i = 0; i < Height; i++) {
        z_buffer[i] = new float[Width];
        std::fill(z_buffer[i], z_buffer[i] + Width, 1.0f);
    }

    // 누적용 벡터
    std::vector<glm::vec3> normal_sum(data.numVertices, glm::vec3(0.0f));
    std::vector<int> normal_count(data.numVertices, 0);

    // 1. 정점 위치와 MV 계산
    for (int i = 0; i < data.numVertices; i++) {
        float vert4[4] = {
            data.vertexBuffer[3 * i + 0],
            data.vertexBuffer[3 * i + 1],
            data.vertexBuffer[3 * i + 2],
            1.0f
        }; 
        float mv[4], cv[4], pv[4], ov[4], sv[4];

        M_Model(mv, vert4, model);
        M_Camera(cv, mv, camera);
        M_Perspective(pv, cv, -0.1f, -1000.0f);
        M_Orthograph(ov, pv, -0.1f, 0.1f, -0.1f, 0.1f, -0.1f, -1000.0f);
        M_Viewport(sv, ov, Width, Height);


        screen_vertices[i] = glm::vec3(sv[0] / sv[3], sv[1] / sv[3], sv[2] / sv[3]);
        w_values[i] = sv[3];

        world_positions[i] = glm::vec3(mv[0] / mv[3], mv[1] / mv[3], mv[2] / mv[3]);
    }

    // 2. 각 삼각형에서 법선 누적
    for (int i = 0; i < data.numTriangles; i++) {
        int k0 = data.indexBuffer[3 * i + 0];
        int k1 = data.indexBuffer[3 * i + 1];
        int k2 = data.indexBuffer[3 * i + 2];

        glm::vec3 p0 = world_positions[k0];
        glm::vec3 p1 = world_positions[k1];
        glm::vec3 p2 = world_positions[k2];

        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        normal_sum[k0] += n; normal_count[k0]++;
        normal_sum[k1] += n; normal_count[k1]++;
        normal_sum[k2] += n; normal_count[k2]++;
    }

    // 3. 평균화된 법선 계산 및 vertex color 설정
    for (int i = 0; i < data.numVertices; i++) {
        glm::vec3 avg_normal = glm::normalize(normal_sum[i] / float(normal_count[i]));
        vertex_normal[i] = avg_normal;

        vertex_colors[i] = calculate_vertex_color(
            world_positions[i], avg_normal, LightSource, view_pos,
            M.kd, M.ka, M.ks, M.p,
            glm::vec3(1.0f), M.Ia
        );
    }


    for (int i = 0; i < data.numTriangles; i++) {
        int k0 = data.indexBuffer[3 * i + 0];
        int k1 = data.indexBuffer[3 * i + 1];
        int k2 = data.indexBuffer[3 * i + 2];

        Vertex v0 = { screen_vertices[k0], w_values[k0], vertex_colors[k0] };
        Vertex v1 = { screen_vertices[k1], w_values[k1], vertex_colors[k1] };
        Vertex v2 = { screen_vertices[k2], w_values[k2], vertex_colors[k2] };

        rasterize_triangle_gouraud_perspective(OutputImage, v0, v1, v2, z_buffer, Width, Height);
    }


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
