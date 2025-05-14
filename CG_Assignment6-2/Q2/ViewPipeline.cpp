#include <stdio.h>
#include <math.h>
#include "camera.h"

void M_Model(float* result, const float* vertex, const float M[4][4])
{
    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += M[i][j] * vertex[j];
        }
    }
}

void M_Camera(float* result, const float* vertex, const Camera& cameraData)
{
    const float* u = cameraData.u;
    const float* v = cameraData.v;
    const float* w = cameraData.w;
    const float* e = cameraData.eye;

    float M[4][4] = {
        {u[0], u[1], u[2], -(u[0] * e[0] + u[1] * e[1] + u[2] * e[2])},
        {v[0], v[1], v[2], -(v[0] * e[0] + v[1] * e[1] + v[2] * e[2])},
        {w[0], w[1], w[2], -(w[0] * e[0] + w[1] * e[1] + w[2] * e[2])},
        {0,    0,    0,     1}
    };

    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += M[i][j] * vertex[j];
        }
    }
}

void M_Orthograph(float* result, const float* vertex, float l, float r, float b, float t, float n, float f)
{
    float M[4][4] = {
        {2 / (r - l), 0,          0,         -(r + l) / (r - l)},
        {0,         2 / (t - b),  0,         -(t + b) / (t - b)},
        {0,         0,         -2 / (f - n), -(f + n) / (f - n)},
        {0,         0,          0,          1}
    };

    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += M[i][j] * vertex[j];
        }
    }
}

void M_Perspective(float* result, const float* vertex, float n, float f)
{
    float M[4][4] = {
        {n, 0, 0, 0},
        {0, n, 0, 0},
        {0, 0, n + f, -f * n},
        {0, 0, 1, 0}
    };

    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += M[i][j] * vertex[j];
        }
    }
}

void M_Viewport(float* result, const float* vertex, int nx, int ny)
{
    float M[4][4] = {
        {nx / 2.0f, 0, 0, (nx - 1) / 2.0f},
        {0, ny / 2.0f, 0, (ny - 1) / 2.0f},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += M[i][j] * vertex[j];
        }
    }
}
